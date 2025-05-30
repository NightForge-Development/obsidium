/* Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (C) 2015-2023 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "generated/chassis.h"

#include "object_lifetime_validation.h"
#include "generated/layer_chassis_dispatch.h"

uint64_t object_track_index = 0;

VulkanTypedHandle ObjTrackStateTypedHandle(const ObjTrackState &track_state) {
    // TODO: Unify Typed Handle representation (i.e. VulkanTypedHandle everywhere there are handle/type pairs)
    VulkanTypedHandle typed_handle;
    typed_handle.handle = track_state.handle;
    typed_handle.type = track_state.object_type;
    return typed_handle;
}

// Destroy memRef lists and free all memory
void ObjectLifetimes::DestroyQueueDataStructures() {
    // Destroy the items in the queue map
    auto snapshot = object_map[kVulkanObjectTypeQueue].snapshot();
    for (const auto &queue : snapshot) {
        uint32_t obj_index = queue.second->object_type;
        assert(num_total_objects > 0);
        num_total_objects--;
        assert(num_objects[obj_index] > 0);
        num_objects[obj_index]--;
        object_map[kVulkanObjectTypeQueue].erase(queue.first);
    }
}

void ObjectLifetimes::DestroyUndestroyedObjects(VulkanObjectType object_type) {
    auto snapshot = object_map[object_type].snapshot();
    for (const auto &item : snapshot) {
        auto object_info = item.second;
        DestroyObjectSilently(object_info->handle, object_type);
    }
}

bool ObjectLifetimes::ValidateAnonymousObject(uint64_t object, VkObjectType core_object_type, const char *invalid_handle_vuid,
                                              const Location &loc) const {
    auto object_type = ConvertCoreObjectToVulkanObject(core_object_type);
    return CheckObjectValidity(object, object_type, invalid_handle_vuid, kVUIDUndefined, loc, kVulkanObjectTypeDevice);
}

void ObjectLifetimes::AllocateCommandBuffer(const VkCommandPool command_pool, const VkCommandBuffer command_buffer,
                                            VkCommandBufferLevel level) {
    auto new_obj_node = std::make_shared<ObjTrackState>();
    new_obj_node->object_type = kVulkanObjectTypeCommandBuffer;
    new_obj_node->handle = HandleToUint64(command_buffer);
    new_obj_node->parent_object = HandleToUint64(command_pool);
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        new_obj_node->status = OBJSTATUS_COMMAND_BUFFER_SECONDARY;
    } else {
        new_obj_node->status = OBJSTATUS_NONE;
    }
    InsertObject(object_map[kVulkanObjectTypeCommandBuffer], command_buffer, kVulkanObjectTypeCommandBuffer, new_obj_node);
    num_objects[kVulkanObjectTypeCommandBuffer]++;
    num_total_objects++;
}

bool ObjectLifetimes::ValidateCommandBuffer(VkCommandPool command_pool, VkCommandBuffer command_buffer, const Location &loc) const {
    bool skip = false;
    uint64_t object_handle = HandleToUint64(command_buffer);
    auto iter = object_map[kVulkanObjectTypeCommandBuffer].find(object_handle);
    if (iter != object_map[kVulkanObjectTypeCommandBuffer].end()) {
        auto node = iter->second;

        if (node->parent_object != HandleToUint64(command_pool)) {
            // We know that the parent *must* be a command pool
            const auto parent_pool = CastFromUint64<VkCommandPool>(node->parent_object);
            const LogObjectList objlist(command_buffer, parent_pool, command_pool);
            skip |= LogError("VUID-vkFreeCommandBuffers-pCommandBuffers-parent", objlist, loc,
                             "attempting to free %s belonging to %s from %s.", FormatHandle(command_buffer).c_str(),
                             FormatHandle(parent_pool).c_str(), FormatHandle(command_pool).c_str());
        }
    } else {
        skip |= LogError("VUID-vkFreeCommandBuffers-pCommandBuffers-00048", command_buffer, loc, "Invalid %s.",
                         FormatHandle(command_buffer).c_str());
    }
    return skip;
}

void ObjectLifetimes::AllocateDescriptorSet(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set) {
    auto new_obj_node = std::make_shared<ObjTrackState>();
    new_obj_node->object_type = kVulkanObjectTypeDescriptorSet;
    new_obj_node->status = OBJSTATUS_NONE;
    new_obj_node->handle = HandleToUint64(descriptor_set);
    new_obj_node->parent_object = HandleToUint64(descriptor_pool);
    InsertObject(object_map[kVulkanObjectTypeDescriptorSet], descriptor_set, kVulkanObjectTypeDescriptorSet, new_obj_node);
    num_objects[kVulkanObjectTypeDescriptorSet]++;
    num_total_objects++;

    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptor_pool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        itr->second->child_objects->insert(HandleToUint64(descriptor_set));
    }
}

bool ObjectLifetimes::ValidateDescriptorSet(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set,
                                            const Location &loc) const {
    bool skip = false;
    uint64_t object_handle = HandleToUint64(descriptor_set);
    auto ds_item = object_map[kVulkanObjectTypeDescriptorSet].find(object_handle);
    if (ds_item != object_map[kVulkanObjectTypeDescriptorSet].end()) {
        if (ds_item->second->parent_object != HandleToUint64(descriptor_pool)) {
            // We know that the parent *must* be a descriptor pool
            const auto parent_pool = CastFromUint64<VkDescriptorPool>(ds_item->second->parent_object);
            const LogObjectList objlist(descriptor_set, parent_pool, descriptor_pool);
            skip |= LogError("VUID-vkFreeDescriptorSets-pDescriptorSets-parent", objlist, loc,
                             "attempting to free %s"
                             " belonging to %s from %s.",
                             FormatHandle(descriptor_set).c_str(), FormatHandle(parent_pool).c_str(),
                             FormatHandle(descriptor_pool).c_str());
        }
    } else {
        skip |= LogError("VUID-vkFreeDescriptorSets-pDescriptorSets-00310", descriptor_set, loc, "Invalid %s.",
                         FormatHandle(descriptor_set).c_str());
    }
    return skip;
}

bool ObjectLifetimes::ValidateDescriptorWrite(VkWriteDescriptorSet const *desc, bool isPush, const Location &loc) const {
    bool skip = false;

    // VkWriteDescriptorSet::dstSet is ignored for push vkCmdPushDescriptorSetKHR, so can be bad handle
    if (!isPush && desc->dstSet) {
        skip |= ValidateObject(desc->dstSet, kVulkanObjectTypeDescriptorSet, false, "VUID-VkWriteDescriptorSet-dstSet-00320",
                               "VUID-VkWriteDescriptorSet-commonparent", loc);
    }

    switch (desc->descriptorType) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
            for (uint32_t i = 0; i < desc->descriptorCount; ++i) {
                skip |=
                    ValidateObject(desc->pTexelBufferView[i], kVulkanObjectTypeBufferView, true,
                                   "VUID-VkWriteDescriptorSet-descriptorType-02994", "VUID-VkWriteDescriptorSet-commonparent", loc);
                if (!null_descriptor_enabled && desc->pTexelBufferView[i] == VK_NULL_HANDLE) {
                    skip |= LogError("VUID-VkWriteDescriptorSet-descriptorType-02995", desc->dstSet, loc,
                                     "texel buffer view must not be VK_NULL_HANDLE.");
                }
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            for (uint32_t i = 0; i < desc->descriptorCount; ++i) {
                skip |= ValidateObject(desc->pImageInfo[i].imageView, kVulkanObjectTypeImageView, true,
                                       "VUID-VkWriteDescriptorSet-descriptorType-02996", "VUID-VkDescriptorImageInfo-commonparent",
                                       loc);
                if (!null_descriptor_enabled && desc->pImageInfo[i].imageView == VK_NULL_HANDLE) {
                    skip |= LogError("VUID-VkWriteDescriptorSet-descriptorType-02997", desc->dstSet, loc,
                                     "image view must not be VK_NULL_HANDLE.");
                }
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
            // Input attachments can never be null
            for (uint32_t i = 0; i < desc->descriptorCount; ++i) {
                skip |= ValidateObject(desc->pImageInfo[i].imageView, kVulkanObjectTypeImageView, false,
                                       "VUID-VkWriteDescriptorSet-descriptorType-07683", "VUID-VkDescriptorImageInfo-commonparent",
                                       loc);
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
            for (uint32_t i = 0; i < desc->descriptorCount; ++i) {
                skip |= ValidateObject(desc->pBufferInfo[i].buffer, kVulkanObjectTypeBuffer, true,
                                       "VUID-VkDescriptorBufferInfo-buffer-parameter", kVUIDUndefined, loc);
                if (!null_descriptor_enabled && desc->pBufferInfo[i].buffer == VK_NULL_HANDLE) {
                    skip |= LogError("VUID-VkDescriptorBufferInfo-buffer-02998", desc->dstSet, loc,
                                     "buffer must not be VK_NULL_HANDLE.");
                }
            }
            break;
        }

        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
            const auto *acc_info = vku::FindStructInPNextChain<VkWriteDescriptorSetAccelerationStructureKHR>(desc->pNext);
            for (uint32_t i = 0; i < desc->descriptorCount; ++i) {
                skip |= ValidateObject(acc_info->pAccelerationStructures[i], kVulkanObjectTypeAccelerationStructureKHR, true,
                                       "VUID-VkWriteDescriptorSetAccelerationStructureKHR-pAccelerationStructures-parameter",
                                       kVUIDUndefined, loc);
            }
            break;
        }
        // TODO - These need to be checked as well
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
        case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
        case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
        case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
        case VK_DESCRIPTOR_TYPE_MAX_ENUM:
            break;
    }

    return skip;
}

bool ObjectLifetimes::PreCallValidateCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                                             VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
                                                             const VkWriteDescriptorSet *pDescriptorWrites,
                                                             const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: commandBuffer: "VUID-vkCmdPushDescriptorSetKHR-commandBuffer-parameter"
    // Checked by chassis: commandBuffer: "VUID-vkCmdPushDescriptorSetKHR-commonparent"
    skip |= ValidateObject(layout, kVulkanObjectTypePipelineLayout, false, "VUID-vkCmdPushDescriptorSetKHR-layout-parameter",
                           "VUID-vkCmdPushDescriptorSetKHR-commonparent", error_obj.location);
    if (pDescriptorWrites) {
        for (uint32_t index0 = 0; index0 < descriptorWriteCount; ++index0) {
            skip |= ValidateDescriptorWrite(&pDescriptorWrites[index0], true, error_obj.location);
        }
    }
    return skip;
}

void ObjectLifetimes::CreateQueue(VkQueue vkObj) {
    std::shared_ptr<ObjTrackState> p_obj_node = NULL;
    auto queue_item = object_map[kVulkanObjectTypeQueue].find(HandleToUint64(vkObj));
    if (queue_item == object_map[kVulkanObjectTypeQueue].end()) {
        p_obj_node = std::make_shared<ObjTrackState>();
        InsertObject(object_map[kVulkanObjectTypeQueue], vkObj, kVulkanObjectTypeQueue, p_obj_node);
        num_objects[kVulkanObjectTypeQueue]++;
        num_total_objects++;
    } else {
        p_obj_node = queue_item->second;
    }
    p_obj_node->object_type = kVulkanObjectTypeQueue;
    p_obj_node->status = OBJSTATUS_NONE;
    p_obj_node->handle = HandleToUint64(vkObj);
}

void ObjectLifetimes::CreateSwapchainImageObject(VkImage swapchain_image, VkSwapchainKHR swapchain) {
    if (!swapchainImageMap.contains(HandleToUint64(swapchain_image))) {
        auto new_obj_node = std::make_shared<ObjTrackState>();
        new_obj_node->object_type = kVulkanObjectTypeImage;
        new_obj_node->status = OBJSTATUS_NONE;
        new_obj_node->handle = HandleToUint64(swapchain_image);
        new_obj_node->parent_object = HandleToUint64(swapchain);
        InsertObject(swapchainImageMap, swapchain_image, kVulkanObjectTypeImage, new_obj_node);
    }
}

bool ObjectLifetimes::ReportLeakedInstanceObjects(VkInstance instance, VulkanObjectType object_type, const std::string &error_code,
                                                  const Location &loc) const {
    bool skip = false;

    auto snapshot = object_map[object_type].snapshot();
    for (const auto &item : snapshot) {
        const auto object_info = item.second;
        const LogObjectList objlist(instance, ObjTrackStateTypedHandle(*object_info));
        skip |= LogError(error_code, objlist, loc, "OBJ ERROR : For %s, %s has not been destroyed.", FormatHandle(instance).c_str(),
                         FormatHandle(ObjTrackStateTypedHandle(*object_info)).c_str());
    }
    return skip;
}

bool ObjectLifetimes::ReportLeakedDeviceObjects(VkDevice device, VulkanObjectType object_type, const std::string &error_code,
                                                const Location &loc) const {
    bool skip = false;

    auto snapshot = object_map[object_type].snapshot();
    for (const auto &item : snapshot) {
        const auto object_info = item.second;
        const LogObjectList objlist(device, ObjTrackStateTypedHandle(*object_info));
        skip |= LogError(error_code, objlist, loc, "OBJ ERROR : For %s, %s has not been destroyed.", FormatHandle(device).c_str(),
                         FormatHandle(ObjTrackStateTypedHandle(*object_info)).c_str());
    }
    return skip;
}

bool ObjectLifetimes::PreCallValidateDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator,
                                                     const ErrorObject &error_obj) const {
    bool skip = false;

    // Checked by chassis: instance: "VUID-vkDestroyInstance-instance-parameter"

    auto snapshot = object_map[kVulkanObjectTypeDevice].snapshot();
    for (const auto &iit : snapshot) {
        auto node = iit.second;

        VkDevice device = reinterpret_cast<VkDevice>(node->handle);
        VkDebugReportObjectTypeEXT debug_object_type = get_debug_report_enum[node->object_type];

        skip |= LogError(kVUID_ObjectTracker_ObjectLeak, instance, error_obj.location,
                         "OBJ ERROR : %s object %s has not been destroyed.", string_VkDebugReportObjectTypeEXT(debug_object_type),
                         FormatHandle(ObjTrackStateTypedHandle(*node)).c_str());

        // Throw errors if any device objects belonging to this instance have not been destroyed
        auto device_layer_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
        auto obj_lifetimes_data = device_layer_data->GetValidationObject<ObjectLifetimes>();
        skip |= obj_lifetimes_data->ReportUndestroyedDeviceObjects(device, error_obj.location);

        skip |= ValidateDestroyObject(device, kVulkanObjectTypeDevice, pAllocator, "VUID-vkDestroyInstance-instance-00630",
                                      "VUID-vkDestroyInstance-instance-00631", error_obj.location);
    }

    skip |= ValidateDestroyObject(instance, kVulkanObjectTypeInstance, pAllocator, "VUID-vkDestroyInstance-instance-00630",
                                  "VUID-vkDestroyInstance-instance-00631", error_obj.location);

    // Report any remaining instance objects
    skip |= ReportUndestroyedInstanceObjects(instance, error_obj.location);

    return skip;
}

bool ObjectLifetimes::PreCallValidateEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                              VkPhysicalDevice *pPhysicalDevices,
                                                              const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: instance: "VUID-vkEnumeratePhysicalDevices-instance-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                             VkPhysicalDevice *pPhysicalDevices, const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pPhysicalDevices) {
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            CreateObject(pPhysicalDevices[i], kVulkanObjectTypePhysicalDevice, nullptr);
        }
    }
}

void ObjectLifetimes::PreCallRecordDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Destroy physical devices
    auto snapshot = object_map[kVulkanObjectTypePhysicalDevice].snapshot();
    for (const auto &iit : snapshot) {
        auto node = iit.second;
        VkPhysicalDevice physical_device = reinterpret_cast<VkPhysicalDevice>(node->handle);
        RecordDestroyObject(physical_device, kVulkanObjectTypePhysicalDevice);
    }

    // Destroy child devices
    auto snapshot2 = object_map[kVulkanObjectTypeDevice].snapshot();
    for (const auto &iit : snapshot2) {
        auto node = iit.second;
        VkDevice device = reinterpret_cast<VkDevice>(node->handle);
        DestroyLeakedInstanceObjects();

        RecordDestroyObject(device, kVulkanObjectTypeDevice);
    }
}

void ObjectLifetimes::PostCallRecordDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator,
                                                    const RecordObject &record_obj) {
    const Location loc(Func::vkDestroyInstance);
    RecordDestroyObject(instance, kVulkanObjectTypeInstance);
}

bool ObjectLifetimes::PreCallValidateDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator,
                                                   const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkDestroyDevice-device-parameter"

    skip |= ValidateDestroyObject(device, kVulkanObjectTypeDevice, pAllocator, "VUID-vkDestroyDevice-device-00379",
                                  "VUID-vkDestroyDevice-device-00380", error_obj.location);
    // Report any remaining objects associated with this VkDevice object in LL
    skip |= ReportUndestroyedDeviceObjects(device, error_obj.location);

    return skip;
}

void ObjectLifetimes::PreCallRecordDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    auto instance_data = GetLayerDataPtr(get_dispatch_key(physical_device), layer_data_map);
    auto object_lifetimes = instance_data->GetValidationObject<ObjectLifetimes>();
    object_lifetimes->RecordDestroyObject(device, kVulkanObjectTypeDevice);
    DestroyLeakedDeviceObjects();

    // Clean up Queue's MemRef Linked Lists
    DestroyQueueDataStructures();
}

bool ObjectLifetimes::PreCallValidateGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex,
                                                    VkQueue *pQueue, const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: device: "VUID-vkGetDeviceQueue-device-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue,
                                                   const RecordObject &record_obj) {
    auto lock = WriteSharedLock();
    CreateQueue(*pQueue);
}

bool ObjectLifetimes::PreCallValidateGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue,
                                                     const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: device: "VUID-vkGetDeviceQueue2-device-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue,
                                                    const RecordObject &record_obj) {
    auto lock = WriteSharedLock();
    CreateQueue(*pQueue);
}

bool ObjectLifetimes::PreCallValidateUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                                          const VkWriteDescriptorSet *pDescriptorWrites,
                                                          uint32_t descriptorCopyCount,
                                                          const VkCopyDescriptorSet *pDescriptorCopies,
                                                          const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkUpdateDescriptorSets-device-parameter"

    if (pDescriptorCopies) {
        for (uint32_t idx0 = 0; idx0 < descriptorCopyCount; ++idx0) {
            if (pDescriptorCopies[idx0].dstSet) {
                skip |= ValidateObject(pDescriptorCopies[idx0].dstSet, kVulkanObjectTypeDescriptorSet, false,
                                       "VUID-VkCopyDescriptorSet-dstSet-parameter", "VUID-VkCopyDescriptorSet-commonparent",
                                       error_obj.location);
            }
            if (pDescriptorCopies[idx0].srcSet) {
                skip |= ValidateObject(pDescriptorCopies[idx0].srcSet, kVulkanObjectTypeDescriptorSet, false,
                                       "VUID-VkCopyDescriptorSet-srcSet-parameter", "VUID-VkCopyDescriptorSet-commonparent",
                                       error_obj.location);
            }
        }
    }
    if (pDescriptorWrites) {
        for (uint32_t idx1 = 0; idx1 < descriptorWriteCount; ++idx1) {
            skip |= ValidateDescriptorWrite(&pDescriptorWrites[idx1], false, error_obj.location);
        }
    }
    return skip;
}

bool ObjectLifetimes::PreCallValidateResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                         VkDescriptorPoolResetFlags flags, const ErrorObject &error_obj) const {
    bool skip = false;
    auto lock = ReadSharedLock();
    // Checked by chassis: device: "VUID-vkResetDescriptorPool-device-parameter"

    skip |= ValidateObject(descriptorPool, kVulkanObjectTypeDescriptorPool, false,
                           "VUID-vkResetDescriptorPool-descriptorPool-parameter",
                           "VUID-vkResetDescriptorPool-descriptorPool-parent", error_obj.location);

    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptorPool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        auto pool_node = itr->second;
        for (auto set : *pool_node->child_objects) {
            skip |= ValidateDestroyObject((VkDescriptorSet)set, kVulkanObjectTypeDescriptorSet, nullptr, kVUIDUndefined,
                                          kVUIDUndefined, error_obj.location);
        }
    }
    return skip;
}

void ObjectLifetimes::PreCallRecordResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                       VkDescriptorPoolResetFlags flags) {
    auto lock = WriteSharedLock();
    // A DescriptorPool's descriptor sets are implicitly deleted when the pool is reset. Remove this pool's descriptor sets from
    // our descriptorSet map.
    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptorPool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        auto pool_node = itr->second;
        for (auto set : *pool_node->child_objects) {
            RecordDestroyObject((VkDescriptorSet)set, kVulkanObjectTypeDescriptorSet);
        }
        pool_node->child_objects->clear();
    }
}

bool ObjectLifetimes::PreCallValidateBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *begin_info,
                                                        const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: commandBuffer: "VUID-vkBeginCommandBuffer-commandBuffer-parameter"

    if (begin_info) {
        auto iter = object_map[kVulkanObjectTypeCommandBuffer].find(HandleToUint64(commandBuffer));
        if (iter != object_map[kVulkanObjectTypeCommandBuffer].end()) {
            auto node = iter->second;
            if ((begin_info->pInheritanceInfo) && (node->status & OBJSTATUS_COMMAND_BUFFER_SECONDARY) &&
                (begin_info->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                skip |= ValidateObject(begin_info->pInheritanceInfo->framebuffer, kVulkanObjectTypeFramebuffer, true,
                                       "VUID-VkCommandBufferBeginInfo-flags-00055",
                                       "VUID-VkCommandBufferInheritanceInfo-commonparent", error_obj.location);
                skip |= ValidateObject(begin_info->pInheritanceInfo->renderPass, kVulkanObjectTypeRenderPass, true,
                                       "VUID-VkCommandBufferBeginInfo-flags-06000",
                                       "VUID-VkCommandBufferInheritanceInfo-commonparent", error_obj.location);
            }
        }
    }
    return skip;
}

bool ObjectLifetimes::PreCallValidateGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain,
                                                           uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages,
                                                           const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkGetSwapchainImagesKHR-device-parameter"

    skip |= ValidateObject(swapchain, kVulkanObjectTypeSwapchainKHR, false, "VUID-vkGetSwapchainImagesKHR-swapchain-parameter",
                           "VUID-vkGetSwapchainImagesKHR-swapchain-parent", error_obj.location);
    return skip;
}

void ObjectLifetimes::PostCallRecordGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                                          VkImage *pSwapchainImages, const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    auto lock = WriteSharedLock();
    if (pSwapchainImages != NULL) {
        for (uint32_t i = 0; i < *pSwapchainImageCount; i++) {
            CreateSwapchainImageObject(pSwapchainImages[i], swapchain);
        }
    }
}

bool ObjectLifetimes::PreCallValidateCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                               const VkAllocationCallbacks *pAllocator,
                                                               VkDescriptorSetLayout *pSetLayout,
                                                               const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkCreateDescriptorSetLayout-device-parameter"

    if (pCreateInfo) {
        if (pCreateInfo->pBindings) {
            for (uint32_t binding_index = 0; binding_index < pCreateInfo->bindingCount; ++binding_index) {
                const VkDescriptorSetLayoutBinding &binding = pCreateInfo->pBindings[binding_index];
                const bool is_sampler_type = binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
                                             binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                if (binding.pImmutableSamplers && is_sampler_type) {
                    for (uint32_t index2 = 0; index2 < binding.descriptorCount; ++index2) {
                        const VkSampler sampler = binding.pImmutableSamplers[index2];
                        skip |= ValidateObject(sampler, kVulkanObjectTypeSampler, false,
                                               "VUID-VkDescriptorSetLayoutBinding-descriptorType-00282", kVUIDUndefined,
                                               error_obj.location);
                    }
                }
            }
        }
    }
    return skip;
}

void ObjectLifetimes::PostCallRecordCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDescriptorSetLayout *pSetLayout, const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    CreateObject(*pSetLayout, kVulkanObjectTypeDescriptorSetLayout, pAllocator);
}

bool ObjectLifetimes::ValidateSamplerObjects(const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const Location &loc) const {
    bool skip = false;
    if (pCreateInfo->pBindings) {
        for (uint32_t index1 = 0; index1 < pCreateInfo->bindingCount; ++index1) {
            for (uint32_t index2 = 0; index2 < pCreateInfo->pBindings[index1].descriptorCount; ++index2) {
                if (pCreateInfo->pBindings[index1].pImmutableSamplers) {
                    skip |= ValidateObject(pCreateInfo->pBindings[index1].pImmutableSamplers[index2], kVulkanObjectTypeSampler,
                                           true, "VUID-VkDescriptorSetLayoutBinding-descriptorType-00282", kVUIDUndefined, loc);
                }
            }
        }
    }
    return skip;
}

bool ObjectLifetimes::PreCallValidateGetDescriptorSetLayoutSupport(VkDevice device,
                                                                   const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                                   VkDescriptorSetLayoutSupport *pSupport,
                                                                   const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkGetDescriptorSetLayoutSupport-device-parameter"

    if (pCreateInfo) {
        skip |= ValidateSamplerObjects(pCreateInfo, error_obj.location);
    }
    return skip;
}
bool ObjectLifetimes::PreCallValidateGetDescriptorSetLayoutSupportKHR(VkDevice device,
                                                                      const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                                      VkDescriptorSetLayoutSupport *pSupport,
                                                                      const ErrorObject &error_obj) const {
    return PreCallValidateGetDescriptorSetLayoutSupport(device, pCreateInfo, pSupport, error_obj);
}

bool ObjectLifetimes::PreCallValidateGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                            uint32_t *pQueueFamilyPropertyCount,
                                                                            VkQueueFamilyProperties *pQueueFamilyProperties,
                                                                            const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetPhysicalDeviceQueueFamilyProperties-physicalDevice-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                           uint32_t *pQueueFamilyPropertyCount,
                                                                           VkQueueFamilyProperties *pQueueFamilyProperties,
                                                                           const RecordObject &record_obj) {}

void ObjectLifetimes::PostCallRecordCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                                   VkInstance *pInstance, const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    CreateObject(*pInstance, kVulkanObjectTypeInstance, pAllocator);
}

bool ObjectLifetimes::PreCallValidateCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkDevice *pDevice,
                                                  const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkCreateDevice-physicalDevice-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkDevice *pDevice,
                                                 const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    CreateObject(*pDevice, kVulkanObjectTypeDevice, pAllocator);

    auto device_data = GetLayerDataPtr(get_dispatch_key(*pDevice), layer_data_map);
    auto object_tracking = device_data->GetValidationObject<ObjectLifetimes>();

    object_tracking->device_createinfo_pnext = SafePnextCopy(pCreateInfo->pNext);
    const auto *robustness2_features =
        vku::FindStructInPNextChain<VkPhysicalDeviceRobustness2FeaturesEXT>(object_tracking->device_createinfo_pnext);
    object_tracking->null_descriptor_enabled = robustness2_features && robustness2_features->nullDescriptor;
}

bool ObjectLifetimes::PreCallValidateAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                                            VkCommandBuffer *pCommandBuffers, const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkAllocateCommandBuffers-device-parameter"

    skip |= ValidateObject(pAllocateInfo->commandPool, kVulkanObjectTypeCommandPool, false,
                           "VUID-VkCommandBufferAllocateInfo-commandPool-parameter", kVUIDUndefined, error_obj.location);
    return skip;
}

void ObjectLifetimes::PostCallRecordAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                                           VkCommandBuffer *pCommandBuffers, const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        AllocateCommandBuffer(pAllocateInfo->commandPool, pCommandBuffers[i], pAllocateInfo->level);
    }
}

bool ObjectLifetimes::PreCallValidateAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                            VkDescriptorSet *pDescriptorSets, const ErrorObject &error_obj) const {
    bool skip = false;
    auto lock = ReadSharedLock();
    // Checked by chassis: device: "VUID-vkAllocateDescriptorSets-device-parameter"

    skip |= ValidateObject(pAllocateInfo->descriptorPool, kVulkanObjectTypeDescriptorPool, false,
                           "VUID-VkDescriptorSetAllocateInfo-descriptorPool-parameter",
                           "VUID-VkDescriptorSetAllocateInfo-commonparent", error_obj.location);
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        skip |= ValidateObject(pAllocateInfo->pSetLayouts[i], kVulkanObjectTypeDescriptorSetLayout, false,
                               "VUID-VkDescriptorSetAllocateInfo-pSetLayouts-parameter",
                               "VUID-VkDescriptorSetAllocateInfo-commonparent", error_obj.location);
    }
    return skip;
}

void ObjectLifetimes::PostCallRecordAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                           VkDescriptorSet *pDescriptorSets, const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    auto lock = WriteSharedLock();
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        AllocateDescriptorSet(pAllocateInfo->descriptorPool, pDescriptorSets[i]);
    }
}

bool ObjectLifetimes::PreCallValidateFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                                        const VkCommandBuffer *pCommandBuffers,
                                                        const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkFreeCommandBuffers-device-parameter"

    skip |= ValidateObject(commandPool, kVulkanObjectTypeCommandPool, false, "VUID-vkFreeCommandBuffers-commandPool-parameter",
                           "VUID-vkFreeCommandBuffers-commandPool-parent", error_obj.location);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        if (pCommandBuffers[i] != VK_NULL_HANDLE) {
            skip |= ValidateCommandBuffer(commandPool, pCommandBuffers[i], error_obj.location);
            skip |= ValidateDestroyObject(pCommandBuffers[i], kVulkanObjectTypeCommandBuffer, nullptr, kVUIDUndefined,
                                          kVUIDUndefined, error_obj.location);
        }
    }
    return skip;
}

void ObjectLifetimes::PreCallRecordFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                                      const VkCommandBuffer *pCommandBuffers) {
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        RecordDestroyObject(pCommandBuffers[i], kVulkanObjectTypeCommandBuffer);
    }
}

bool ObjectLifetimes::PreCallValidateDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                                                         const VkAllocationCallbacks *pAllocator,
                                                         const ErrorObject &error_obj) const {
    bool skip = false;
    skip |= ValidateObject(swapchain, kVulkanObjectTypeSwapchainKHR, true, "VUID-vkDestroySwapchainKHR-swapchain-parameter",
                           "VUID-vkDestroySwapchainKHR-swapchain-parent", error_obj.location);
    skip |=
        ValidateDestroyObject(swapchain, kVulkanObjectTypeSwapchainKHR, pAllocator, "VUID-vkDestroySwapchainKHR-swapchain-01283",
                              "VUID-vkDestroySwapchainKHR-swapchain-01284", error_obj.location);
    return skip;
}

void ObjectLifetimes::PreCallRecordDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
                                                       const VkAllocationCallbacks *pAllocator) {
    RecordDestroyObject(swapchain, kVulkanObjectTypeSwapchainKHR);

    auto snapshot = swapchainImageMap.snapshot(
        [swapchain](const std::shared_ptr<ObjTrackState> &pNode) { return pNode->parent_object == HandleToUint64(swapchain); });
    for (const auto &itr : snapshot) {
        swapchainImageMap.erase(itr.first);
    }
}

bool ObjectLifetimes::PreCallValidateFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool,
                                                        uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets,
                                                        const ErrorObject &error_obj) const {
    auto lock = ReadSharedLock();
    bool skip = false;
    // Checked by chassis: device: "VUID-vkFreeDescriptorSets-device-parameter"

    skip |=
        ValidateObject(descriptorPool, kVulkanObjectTypeDescriptorPool, false, "VUID-vkFreeDescriptorSets-descriptorPool-parameter",
                       "VUID-vkFreeDescriptorSets-descriptorPool-parent", error_obj.location);
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        if (pDescriptorSets[i] != VK_NULL_HANDLE) {
            skip |= ValidateDescriptorSet(descriptorPool, pDescriptorSets[i], error_obj.location);
            skip |= ValidateDestroyObject(pDescriptorSets[i], kVulkanObjectTypeDescriptorSet, nullptr, kVUIDUndefined,
                                          kVUIDUndefined, error_obj.location);
        }
    }
    return skip;
}
void ObjectLifetimes::PreCallRecordFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                                                      const VkDescriptorSet *pDescriptorSets) {
    auto lock = WriteSharedLock();
    std::shared_ptr<ObjTrackState> pool_node = nullptr;
    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptorPool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        pool_node = itr->second;
    }
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        RecordDestroyObject(pDescriptorSets[i], kVulkanObjectTypeDescriptorSet);
        if (pool_node) {
            pool_node->child_objects->erase(HandleToUint64(pDescriptorSets[i]));
        }
    }
}

bool ObjectLifetimes::PreCallValidateDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                           const VkAllocationCallbacks *pAllocator,
                                                           const ErrorObject &error_obj) const {
    auto lock = ReadSharedLock();
    bool skip = false;
    // Checked by chassis: device: "VUID-vkDestroyDescriptorPool-device-parameter"

    skip |= ValidateObject(descriptorPool, kVulkanObjectTypeDescriptorPool, true,
                           "VUID-vkDestroyDescriptorPool-descriptorPool-parameter",
                           "VUID-vkDestroyDescriptorPool-descriptorPool-parent", error_obj.location);

    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptorPool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        auto pool_node = itr->second;
        for (auto set : *pool_node->child_objects) {
            skip |= ValidateDestroyObject((VkDescriptorSet)set, kVulkanObjectTypeDescriptorSet, nullptr, kVUIDUndefined,
                                          kVUIDUndefined, error_obj.location);
        }
    }
    skip |= ValidateDestroyObject(descriptorPool, kVulkanObjectTypeDescriptorPool, pAllocator,
                                  "VUID-vkDestroyDescriptorPool-descriptorPool-00304",
                                  "VUID-vkDestroyDescriptorPool-descriptorPool-00305", error_obj.location);
    return skip;
}
void ObjectLifetimes::PreCallRecordDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                         const VkAllocationCallbacks *pAllocator) {
    auto lock = WriteSharedLock();
    auto itr = object_map[kVulkanObjectTypeDescriptorPool].find(HandleToUint64(descriptorPool));
    if (itr != object_map[kVulkanObjectTypeDescriptorPool].end()) {
        auto pool_node = itr->second;
        for (auto set : *pool_node->child_objects) {
            RecordDestroyObject((VkDescriptorSet)set, kVulkanObjectTypeDescriptorSet);
        }
        pool_node->child_objects->clear();
    }
    RecordDestroyObject(descriptorPool, kVulkanObjectTypeDescriptorPool);
}

bool ObjectLifetimes::PreCallValidateDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                                                        const VkAllocationCallbacks *pAllocator,
                                                        const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkDestroyCommandPool-device-parameter"

    skip |= ValidateObject(commandPool, kVulkanObjectTypeCommandPool, true, "VUID-vkDestroyCommandPool-commandPool-parameter",
                           "VUID-vkDestroyCommandPool-commandPool-parent", error_obj.location);

    auto snapshot = object_map[kVulkanObjectTypeCommandBuffer].snapshot(
        [commandPool](const std::shared_ptr<ObjTrackState> &pNode) { return pNode->parent_object == HandleToUint64(commandPool); });
    for (const auto &itr : snapshot) {
        auto node = itr.second;
        skip |= ValidateCommandBuffer(commandPool, reinterpret_cast<VkCommandBuffer>(itr.first), error_obj.location);
        skip |= ValidateDestroyObject(reinterpret_cast<VkCommandBuffer>(itr.first), kVulkanObjectTypeCommandBuffer, nullptr,
                                      kVUIDUndefined, kVUIDUndefined, error_obj.location);
    }
    skip |=
        ValidateDestroyObject(commandPool, kVulkanObjectTypeCommandPool, pAllocator, "VUID-vkDestroyCommandPool-commandPool-00042",
                              "VUID-vkDestroyCommandPool-commandPool-00043", error_obj.location);
    return skip;
}

void ObjectLifetimes::PreCallRecordDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                                                      const VkAllocationCallbacks *pAllocator) {
    auto snapshot = object_map[kVulkanObjectTypeCommandBuffer].snapshot(
        [commandPool](const std::shared_ptr<ObjTrackState> &pNode) { return pNode->parent_object == HandleToUint64(commandPool); });
    // A CommandPool's cmd buffers are implicitly deleted when pool is deleted. Remove this pool's cmdBuffers from cmd buffer map.
    for (const auto &itr : snapshot) {
        RecordDestroyObject(reinterpret_cast<VkCommandBuffer>(itr.first), kVulkanObjectTypeCommandBuffer);
    }
    RecordDestroyObject(commandPool, kVulkanObjectTypeCommandPool);
}

bool ObjectLifetimes::PreCallValidateGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
                                                                             uint32_t *pQueueFamilyPropertyCount,
                                                                             VkQueueFamilyProperties2 *pQueueFamilyProperties,
                                                                             const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetPhysicalDeviceQueueFamilyProperties2-physicalDevice-parameter"
    return skip;
}

bool ObjectLifetimes::PreCallValidateGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                                uint32_t *pQueueFamilyPropertyCount,
                                                                                VkQueueFamilyProperties2 *pQueueFamilyProperties,
                                                                                const ErrorObject &error_obj) const {
    return PreCallValidateGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties,
                                                                  error_obj);
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
                                                                            uint32_t *pQueueFamilyPropertyCount,
                                                                            VkQueueFamilyProperties2 *pQueueFamilyProperties,
                                                                            const RecordObject &record_obj) {}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                               uint32_t *pQueueFamilyPropertyCount,
                                                                               VkQueueFamilyProperties2 *pQueueFamilyProperties,
                                                                               const RecordObject &record_obj) {}

bool ObjectLifetimes::PreCallValidateGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice,
                                                                           uint32_t *pPropertyCount,
                                                                           VkDisplayPropertiesKHR *pProperties,
                                                                           const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetPhysicalDeviceDisplayPropertiesKHR-physicalDevice-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                                          VkDisplayPropertiesKHR *pProperties,
                                                                          const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            CreateObject(pProperties[i].display, kVulkanObjectTypeDisplayKHR, nullptr);
        }
    }
}

bool ObjectLifetimes::PreCallValidateGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                                 uint32_t *pPropertyCount, VkDisplayModePropertiesKHR *pProperties,
                                                                 const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetDisplayModePropertiesKHR-physicalDevice-parameter"

    skip |=
        ValidateObject(display, kVulkanObjectTypeDisplayKHR, false, "VUID-vkGetDisplayModePropertiesKHR-display-parameter",
                       "VUID-vkGetDisplayModePropertiesKHR-display-parent", error_obj.location, kVulkanObjectTypePhysicalDevice);

    return skip;
}

void ObjectLifetimes::PostCallRecordGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                                uint32_t *pPropertyCount, VkDisplayModePropertiesKHR *pProperties,
                                                                const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            CreateObject(pProperties[i].displayMode, kVulkanObjectTypeDisplayModeKHR, nullptr);
        }
    }
}

bool ObjectLifetimes::PreCallValidateGetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                            uint32_t *pPropertyCount,
                                                                            VkDisplayProperties2KHR *pProperties,
                                                                            const ErrorObject &error_obj) const {
    constexpr bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetPhysicalDeviceDisplayProperties2KHR-physicalDevice-parameter"
    return skip;
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                           uint32_t *pPropertyCount,
                                                                           VkDisplayProperties2KHR *pProperties,
                                                                           const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t index = 0; index < *pPropertyCount; ++index) {
            CreateObject(pProperties[index].displayProperties.display, kVulkanObjectTypeDisplayKHR, nullptr);
        }
    }
}

bool ObjectLifetimes::PreCallValidateGetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                                  uint32_t *pPropertyCount,
                                                                  VkDisplayModeProperties2KHR *pProperties,
                                                                  const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: physicalDevice: "VUID-vkGetDisplayModeProperties2KHR-physicalDevice-parameter"

    skip |=
        ValidateObject(display, kVulkanObjectTypeDisplayKHR, false, "VUID-vkGetDisplayModeProperties2KHR-display-parameter",
                       "VUID-vkGetDisplayModeProperties2KHR-display-parent", error_obj.location, kVulkanObjectTypePhysicalDevice);

    return skip;
}

void ObjectLifetimes::PostCallRecordGetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                                                 uint32_t *pPropertyCount, VkDisplayModeProperties2KHR *pProperties,
                                                                 const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t index = 0; index < *pPropertyCount; ++index) {
            CreateObject(pProperties[index].displayModeProperties.displayMode, kVulkanObjectTypeDisplayModeKHR, nullptr);
        }
    }
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice,
                                                                               uint32_t *pPropertyCount,
                                                                               VkDisplayPlanePropertiesKHR *pProperties,
                                                                               const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t index = 0; index < *pPropertyCount; ++index) {
            CreateObject(pProperties[index].currentDisplay, kVulkanObjectTypeDisplayKHR, nullptr);
        }
    }
}

void ObjectLifetimes::PostCallRecordGetPhysicalDeviceDisplayPlaneProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                                uint32_t *pPropertyCount,
                                                                                VkDisplayPlaneProperties2KHR *pProperties,
                                                                                const RecordObject &record_obj) {
    if ((record_obj.result != VK_SUCCESS) && (record_obj.result != VK_INCOMPLETE)) return;
    if (pProperties) {
        for (uint32_t index = 0; index < *pPropertyCount; ++index) {
            CreateObject(pProperties[index].displayPlaneProperties.currentDisplay, kVulkanObjectTypeDisplayKHR, nullptr);
        }
    }
}

bool ObjectLifetimes::PreCallValidateCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer,
                                                       const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkCreateFramebuffer-device-parameter"

    if (pCreateInfo) {
        skip |= ValidateObject(pCreateInfo->renderPass, kVulkanObjectTypeRenderPass, false,
                               "VUID-VkFramebufferCreateInfo-renderPass-parameter", "VUID-VkFramebufferCreateInfo-commonparent",
                               error_obj.location);
        if ((pCreateInfo->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT) == 0) {
            for (uint32_t index1 = 0; index1 < pCreateInfo->attachmentCount; ++index1) {
                skip |= ValidateObject(pCreateInfo->pAttachments[index1], kVulkanObjectTypeImageView, true,
                                       "VUID-VkFramebufferCreateInfo-flags-02778", "VUID-VkFramebufferCreateInfo-commonparent",
                                       error_obj.location);
            }
        }
    }

    return skip;
}

void ObjectLifetimes::PostCallRecordCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                      const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer,
                                                      const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    CreateObject(*pFramebuffer, kVulkanObjectTypeFramebuffer, pAllocator);
}

bool ObjectLifetimes::PreCallValidateSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo,
                                                                const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkSetDebugUtilsObjectNameEXT-device-parameter"

    skip |= ValidateAnonymousObject(pNameInfo->objectHandle, pNameInfo->objectType,
                                    "VUID-VkDebugUtilsObjectNameInfoEXT-objectType-02590", error_obj.location);

    return skip;
}

bool ObjectLifetimes::PreCallValidateSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo,
                                                               const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkSetDebugUtilsObjectTagEXT-device-parameter"

    skip |= ValidateAnonymousObject(pTagInfo->objectHandle, pTagInfo->objectType,
                                    "VUID-VkDebugUtilsObjectTagInfoEXT-objectHandle-01910", error_obj.location);

    return skip;
}

bool ObjectLifetimes::PreCallValidateCreateDescriptorUpdateTemplate(VkDevice device,
                                                                    const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                                    const VkAllocationCallbacks *pAllocator,
                                                                    VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate,
                                                                    const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkCreateDescriptorUpdateTemplate-device-parameter"

    if (pCreateInfo) {
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET) {
            skip |= ValidateObject(pCreateInfo->descriptorSetLayout, kVulkanObjectTypeDescriptorSetLayout, false,
                                   "VUID-VkDescriptorUpdateTemplateCreateInfo-templateType-00350",
                                   "VUID-VkDescriptorUpdateTemplateCreateInfo-commonparent", error_obj.location);
        }
        if (pCreateInfo->templateType == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR) {
            skip |= ValidateObject(pCreateInfo->pipelineLayout, kVulkanObjectTypePipelineLayout, false,
                                   "VUID-VkDescriptorUpdateTemplateCreateInfo-templateType-00352",
                                   "VUID-VkDescriptorUpdateTemplateCreateInfo-commonparent", error_obj.location);
        }
    }

    return skip;
}

bool ObjectLifetimes::PreCallValidateCreateDescriptorUpdateTemplateKHR(VkDevice device,
                                                                       const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                                       const VkAllocationCallbacks *pAllocator,
                                                                       VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate,
                                                                       const ErrorObject &error_obj) const {
    return PreCallValidateCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate, error_obj);
}

void ObjectLifetimes::PostCallRecordCreateDescriptorUpdateTemplate(VkDevice device,
                                                                   const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                                   const VkAllocationCallbacks *pAllocator,
                                                                   VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate,
                                                                   const RecordObject &record_obj) {
    if (record_obj.result != VK_SUCCESS) return;
    CreateObject(*pDescriptorUpdateTemplate, kVulkanObjectTypeDescriptorUpdateTemplate, pAllocator);
}

void ObjectLifetimes::PostCallRecordCreateDescriptorUpdateTemplateKHR(VkDevice device,
                                                                      const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
                                                                      const VkAllocationCallbacks *pAllocator,
                                                                      VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate,
                                                                      const RecordObject &record_obj) {
    return PostCallRecordCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate, record_obj);
}

bool ObjectLifetimes::ValidateAccelerationStructures(const char *dst_handle_vuid, uint32_t count,
                                                     const VkAccelerationStructureBuildGeometryInfoKHR *infos,
                                                     const Location &loc) const {
    bool skip = false;
    if (infos) {
        const char *device_vuid = "VUID-VkAccelerationStructureBuildGeometryInfoKHR-commonparent";
        for (uint32_t i = 0; i < count; ++i) {
            skip |= ValidateObject(infos[i].srcAccelerationStructure, kVulkanObjectTypeAccelerationStructureKHR, true,
                                   kVUIDUndefined, device_vuid, loc);
            skip |= ValidateObject(infos[i].dstAccelerationStructure, kVulkanObjectTypeAccelerationStructureKHR, false,
                                   dst_handle_vuid, device_vuid, loc);
        }
    }

    return skip;
}

bool ObjectLifetimes::PreCallValidateCmdBuildAccelerationStructuresKHR(
    VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos, const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: commandBuffer: "VUID-vkCmdBuildAccelerationStructuresKHR-commandBuffer-parameter"

    skip |= ValidateAccelerationStructures("VUID-vkCmdBuildAccelerationStructuresKHR-dstAccelerationStructure-03800", infoCount,
                                           pInfos, error_obj.location);
    return skip;
}

bool ObjectLifetimes::PreCallValidateCmdBuildAccelerationStructuresIndirectKHR(
    VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
    const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t *const *ppMaxPrimitiveCounts,
    const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: commandBuffer: "VUID-vkCmdBuildAccelerationStructuresIndirectKHR-commandBuffer-parameter"

    skip |= ValidateAccelerationStructures("VUID-vkCmdBuildAccelerationStructuresIndirectKHR-dstAccelerationStructure-03800",
                                           infoCount, pInfos, error_obj.location);
    return skip;
}

bool ObjectLifetimes::PreCallValidateBuildAccelerationStructuresKHR(
    VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos, const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkBuildAccelerationStructuresKHR-device-parameter"

    skip |= ValidateObject(deferredOperation, kVulkanObjectTypeDeferredOperationKHR, true,
                           "VUID-vkBuildAccelerationStructuresKHR-deferredOperation-parameter",
                           "VUID-vkBuildAccelerationStructuresKHR-deferredOperation-parent", error_obj.location);
    skip |= ValidateAccelerationStructures("VUID-vkBuildAccelerationStructuresKHR-dstAccelerationStructure-03800", infoCount,
                                           pInfos, error_obj.location);
    return skip;
}

bool ObjectLifetimes::PreCallValidateCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation,
                                                                  VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                                  const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
                                                                  const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines,
                                                                  const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkCreateRayTracingPipelinesKHR-device-parameter"

    skip |= ValidateObject(deferredOperation, kVulkanObjectTypeDeferredOperationKHR, true,
                           "VUID-vkCreateRayTracingPipelinesKHR-deferredOperation-parameter",
                           "VUID-vkCreateRayTracingPipelinesKHR-deferredOperation-parent", error_obj.location);
    skip |= ValidateObject(pipelineCache, kVulkanObjectTypePipelineCache, true,
                           "VUID-vkCreateRayTracingPipelinesKHR-pipelineCache-parameter",
                           "VUID-vkCreateRayTracingPipelinesKHR-pipelineCache-parent", error_obj.location);
    if (pCreateInfos) {
        for (uint32_t index0 = 0; index0 < createInfoCount; ++index0) {
            if (pCreateInfos[index0].pStages) {
                for (uint32_t index1 = 0; index1 < pCreateInfos[index0].stageCount; ++index1) {
                    skip |=
                        ValidateObject(pCreateInfos[index0].pStages[index1].module, kVulkanObjectTypeShaderModule, true,
                                       "VUID-VkPipelineShaderStageCreateInfo-module-parameter", kVUIDUndefined, error_obj.location);
                }
            }
            if (pCreateInfos[index0].pLibraryInfo) {
                if (pCreateInfos[index0].pLibraryInfo->pLibraries) {
                    for (uint32_t index2 = 0; index2 < pCreateInfos[index0].pLibraryInfo->libraryCount; ++index2) {
                        skip |= ValidateObject(pCreateInfos[index0].pLibraryInfo->pLibraries[index2], kVulkanObjectTypePipeline,
                                               false, "VUID-VkPipelineLibraryCreateInfoKHR-pLibraries-parameter", kVUIDUndefined,
                                               error_obj.location);
                    }
                }
            }
            skip |= ValidateObject(pCreateInfos[index0].layout, kVulkanObjectTypePipelineLayout, false,
                                   "VUID-VkRayTracingPipelineCreateInfoKHR-layout-parameter",
                                   "VUID-VkRayTracingPipelineCreateInfoKHR-commonparent", error_obj.location);
            if ((pCreateInfos[index0].flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) && (pCreateInfos[index0].basePipelineIndex == -1))
                skip |= ValidateObject(pCreateInfos[index0].basePipelineHandle, kVulkanObjectTypePipeline, false,
                                       "VUID-VkRayTracingPipelineCreateInfoKHR-flags-07984",
                                       "VUID-VkRayTracingPipelineCreateInfoKHR-commonparent", error_obj.location);
        }
    }

    return skip;
}

void ObjectLifetimes::PostCallRecordCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation,
                                                                 VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                                                 const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
                                                                 const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines,
                                                                 const RecordObject &record_obj) {
    if (VK_ERROR_VALIDATION_FAILED_EXT == record_obj.result) return;
    if (pPipelines) {
        if (deferredOperation != VK_NULL_HANDLE && record_obj.result == VK_OPERATION_DEFERRED_KHR) {
            auto register_fn = [this, pAllocator](const std::vector<VkPipeline> &pipelines) {
                for (auto pipe : pipelines) {
                    if (!pipe) continue;
                    this->CreateObject(pipe, kVulkanObjectTypePipeline, pAllocator);
                }
            };

            auto layer_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
            if (wrap_handles) {
                deferredOperation = layer_data->Unwrap(deferredOperation);
            }
            std::vector<std::function<void(const std::vector<VkPipeline> &)>> cleanup_fn;
            auto find_res = layer_data->deferred_operation_post_check.pop(deferredOperation);
            if (find_res->first) {
                cleanup_fn = std::move(find_res->second);
            }
            cleanup_fn.emplace_back(register_fn);
            layer_data->deferred_operation_post_check.insert(deferredOperation, cleanup_fn);
        } else {
            for (uint32_t index = 0; index < createInfoCount; index++) {
                if (!pPipelines[index]) continue;
                CreateObject(pPipelines[index], kVulkanObjectTypePipeline, pAllocator);
            }
        }
    }
}
#ifdef VK_USE_PLATFORM_METAL_EXT
bool ObjectLifetimes::PreCallValidateExportMetalObjectsEXT(VkDevice device, VkExportMetalObjectsInfoEXT *pMetalObjectsInfo,
                                                           const ErrorObject &error_obj) const {
    bool skip = false;
    // Checked by chassis: device: "VUID-vkExportMetalObjectsEXT-device-parameter"

    const VkBaseOutStructure *metal_objects_info_ptr = reinterpret_cast<const VkBaseOutStructure *>(pMetalObjectsInfo->pNext);
    while (metal_objects_info_ptr) {
        switch (metal_objects_info_ptr->sType) {
            case VK_STRUCTURE_TYPE_EXPORT_METAL_COMMAND_QUEUE_INFO_EXT: {
                auto metal_command_queue_ptr = reinterpret_cast<const VkExportMetalCommandQueueInfoEXT *>(metal_objects_info_ptr);
                skip |= ValidateObject(metal_command_queue_ptr->queue, kVulkanObjectTypeQueue, false,
                                       "VUID-VkExportMetalCommandQueueInfoEXT-queue-parameter", kVUIDUndefined, error_obj.location);
            } break;
            case VK_STRUCTURE_TYPE_EXPORT_METAL_BUFFER_INFO_EXT: {
                auto metal_buffer_ptr = reinterpret_cast<const VkExportMetalBufferInfoEXT *>(metal_objects_info_ptr);
                skip |= ValidateObject(metal_buffer_ptr->memory, kVulkanObjectTypeDeviceMemory, false,
                                       "VUID-VkExportMetalBufferInfoEXT-memory-parameter", kVUIDUndefined, error_obj.location);
            } break;
            case VK_STRUCTURE_TYPE_EXPORT_METAL_TEXTURE_INFO_EXT: {
                auto metal_texture_ptr = reinterpret_cast<const VkExportMetalTextureInfoEXT *>(metal_objects_info_ptr);
                skip |= ValidateObject(metal_texture_ptr->image, kVulkanObjectTypeImage, true,
                                       "VUID-VkExportMetalTextureInfoEXT-image-parameter",
                                       "VUID-VkExportMetalTextureInfoEXT-commonparent", error_obj.location);
                skip |= ValidateObject(metal_texture_ptr->imageView, kVulkanObjectTypeImageView, true,
                                       "VUID-VkExportMetalTextureInfoEXT-imageView-parameter",
                                       "VUID-VkExportMetalTextureInfoEXT-commonparent", error_obj.location);
                skip |= ValidateObject(metal_texture_ptr->bufferView, kVulkanObjectTypeBufferView, true,
                                       "VUID-VkExportMetalTextureInfoEXT-bufferView-parameter",
                                       "VUID-VkExportMetalTextureInfoEXT-commonparent", error_obj.location);
            } break;
            case VK_STRUCTURE_TYPE_EXPORT_METAL_IO_SURFACE_INFO_EXT: {
                auto metal_iosurface_ptr = reinterpret_cast<const VkExportMetalIOSurfaceInfoEXT *>(metal_objects_info_ptr);
                skip |= ValidateObject(metal_iosurface_ptr->image, kVulkanObjectTypeImage, false,
                                       "VUID-VkExportMetalIOSurfaceInfoEXT-image-parameter", kVUIDUndefined, error_obj.location);
            } break;
            case VK_STRUCTURE_TYPE_EXPORT_METAL_SHARED_EVENT_INFO_EXT: {
                auto metal_shared_event_ptr = reinterpret_cast<const VkExportMetalSharedEventInfoEXT *>(metal_objects_info_ptr);
                skip |= ValidateObject(metal_shared_event_ptr->semaphore, kVulkanObjectTypeSemaphore, true,
                                       "VUID-VkExportMetalSharedEventInfoEXT-semaphore-parameter",
                                       "VUID-VkExportMetalSharedEventInfoEXT-commonparent", error_obj.location);
                skip |= ValidateObject(metal_shared_event_ptr->event, kVulkanObjectTypeEvent, true,
                                       "VUID-VkExportMetalSharedEventInfoEXT-event-parameter",
                                       "VUID-VkExportMetalSharedEventInfoEXT-commonparent", error_obj.location);

            } break;
            default:
                break;
        }
        metal_objects_info_ptr = metal_objects_info_ptr->pNext;
    }
    return skip;
}
#endif  //  VK_USE_PLATFORM_METAL_EXT

bool ObjectLifetimes::PreCallValidateGetDescriptorEXT(VkDevice device, const VkDescriptorGetInfoEXT *pDescriptorInfo,
                                                      size_t dataSize, void *pDescriptor, const ErrorObject &error_obj) const {
    bool skip = false;
    skip |= ValidateObject(device, kVulkanObjectTypeDevice, false, kVUIDUndefined, kVUIDUndefined, error_obj.location);

    return skip;
}
