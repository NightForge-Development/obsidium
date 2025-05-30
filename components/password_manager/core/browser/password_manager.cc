// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_manager.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/types/optional_util.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/logging/log_manager.h"
#include "components/autofill/core/common/form_data_predictions.h"
#include "components/autofill/core/common/password_generation_util.h"
#include "components/autofill/core/common/save_password_progress_logger.h"
#include "components/autofill/core/common/signatures.h"
#include "components/password_manager/core/browser/affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/credential_cache.h"
#include "components/password_manager/core/browser/field_info_manager.h"
#include "components/password_manager/core/browser/leak_detection/leak_detection_request_utils.h"
#include "components/password_manager/core/browser/origin_credential_store.h"
#include "components/password_manager/core/browser/password_autofill_manager.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/password_manager/core/browser/password_generation_frame_helper.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/browser/password_reuse_manager.h"
#include "components/password_manager/core/browser/password_save_manager_impl.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/password_manager/core/common/password_manager_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#if BUILDFLAG(IS_WIN)
#include "components/prefs/pref_registry_simple.h"
#endif  // BUILDFLAG(IS_WIN)

using autofill::ACCOUNT_CREATION_PASSWORD;
using autofill::CalculateFormSignature;
using autofill::FieldDataManager;
using autofill::FieldRendererId;
using autofill::FormData;
using autofill::FormRendererId;
using autofill::NEW_PASSWORD;
using autofill::UNKNOWN_TYPE;
using autofill::USERNAME;
using autofill::mojom::SubmissionIndicatorEvent;
using base::NumberToString;
using BlocklistedStatus =
    password_manager::OriginCredentialStore::BlocklistedStatus;

namespace password_manager {

namespace {

// Shorten the name to spare line breaks. The code provides enough context
// already.
using Logger = autofill::SavePasswordProgressLogger;

bool AreChangePasswordFieldsEmpty(const FormData& form_data,
                                  const PasswordForm& parsed_form) {
  const std::u16string& old_password = parsed_form.password_element;
  const std::u16string& new_password = parsed_form.new_password_element;
  const std::u16string& confirmation_password =
      parsed_form.confirmation_password_element;
  for (const auto& field : form_data.fields) {
    if (!field.value.empty() &&
        (field.name == new_password ||
         (!old_password.empty() && field.name == old_password) ||
         (!confirmation_password.empty() &&
          field.name == confirmation_password))) {
      return false;
    }
  }
  return true;
}

// Returns true if the user needs to be prompted before a password can be
// saved (instead of automatically saving the password), based on inspecting
// the state of |manager|.
bool ShouldPromptUserToSavePassword(const PasswordFormManager& manager) {
  if (manager.IsPasswordUpdate()) {
    // Updating a credential might erase a useful stored value by accident.
    // Always ask the user to confirm.
    return true;
  }

  if (manager.HasGeneratedPassword()) {
    return false;
  }

  const auto& form = manager.GetPendingCredentials();
  if (form.match_type.has_value()) {
    switch (password_manager_util::GetMatchType(form)) {
      case password_manager_util::GetLoginMatchType::kExact:
        break;
      case password_manager_util::GetLoginMatchType::kAffiliated:
        // User successfully signed-in with affiliated web credentials. These
        // credentials should be automatically saved in order to be autofilled
        // on next login.
        if (!IsValidAndroidFacetURI(
                manager.GetPendingCredentials().signon_realm)) {
          return false;
        }
        break;
      case password_manager_util::GetLoginMatchType::kPSL:
        // User successfully signed-in with PSL match credentials. These
        // credentials should be automatically saved in order to be autofilled
        // on next login.
        return false;
    }
  }

  return manager.IsNewLogin();
}

bool ShouldShowManualFallbackForGeneratedPassword(
    const PasswordFormManager& manager) {
#if !BUILDFLAG(IS_IOS)
  // On non-iOS manual fallback menu shows a confirmation that the
  // generated password is presaved.
  return manager.HasGeneratedPassword();
#else
  // On iOS manual fallback menu is only used to edit the credential,
  // and is not applicable to generated passwords.
  return false;
#endif  // !BUILDFLAG(IS_IOS)
}

#if BUILDFLAG(USE_BLINK)
// Finds the matched form manager with id |form_renderer_id| in
// |form_managers|.
PasswordFormManager* FindMatchedManagerByRendererId(
    autofill::FormRendererId form_renderer_id,
    const std::vector<std::unique_ptr<PasswordFormManager>>& form_managers,
    const PasswordManagerDriver* driver) {
  for (const auto& form_manager : form_managers) {
    if (form_manager->DoesManage(form_renderer_id, driver))
      return form_manager.get();
  }
  return nullptr;
}
#endif  // BUILDFLAG(USE_BLINK)

bool HasSingleUsernameVote(const FormPredictions& form) {
  return base::ranges::any_of(
      form.fields,
      [](const auto& type) {
        return password_manager_util::IsSingleUsernameType(type);
      },
      &PasswordFieldPrediction::type);
}

// Returns true if at least one of the fields in |form| has a prediction to be a
// new-password related field.
bool HasNewPasswordVote(const FormPredictions& form) {
  auto is_creation_password_or_new_password = [](const auto& type) {
    return type == ACCOUNT_CREATION_PASSWORD || type == NEW_PASSWORD;
  };

  return base::ranges::any_of(form.fields, is_creation_password_or_new_password,
                              &PasswordFieldPrediction::type);
}

// Returns true iff `form` is not recognized as a password form by the renderer,
// but contains either a field for a username first flow, a password reset flow,
// or a clear-text password field.
bool IsRelevantForPasswordManagerButNotRecognizedByRenderer(
    const FormData& form,
    const FormPredictions& form_predictions) {
  if (util::IsRendererRecognizedCredentialForm(form)) {
    return false;
  }
  // It is relevant for PWM if it either contains a field for a username
  // first flow or a clear-text password field.
  return HasSingleUsernameVote(form_predictions) ||
         HasNewPasswordVote(form_predictions);
}

bool IsMutedInsecureCredential(const PasswordForm* credential,
                               InsecureType insecure_type) {
  auto it = credential->password_issues.find(insecure_type);
  return it != credential->password_issues.end() && it->second.is_muted;
}

bool HasMutedCredentials(const std::vector<const PasswordForm*>& credentials,
                         const std::u16string& username) {
  return base::ranges::any_of(credentials, [&username](const auto& credential) {
    return credential->username_value == username &&
           (IsMutedInsecureCredential(credential, InsecureType::kLeaked) ||
            IsMutedInsecureCredential(credential, InsecureType::kPhished));
  });
}

// Returns true iff a password field is absent or hidden.
bool IsSingleUsernameSubmission(const PasswordForm& submitted_form) {
  if (submitted_form.IsSingleUsername())
    return true;

  for (auto const& field : submitted_form.form_data.fields) {
    if (submitted_form.password_element_renderer_id ==
            field.unique_renderer_id ||
        submitted_form.new_password_element_renderer_id ==
            field.unique_renderer_id) {
      if (field.is_focusable)
        return false;
    }
  }
  return true;
}

// A helper just so the subscription field in PasswordManager can be const.
// `manager` must outlive the returned subscription.
base::CallbackListSubscription AddSyncEnabledOrDisabledCallback(
    PasswordManager* manager,
    PasswordManagerClient* client) {
  if (PasswordStoreInterface* account_store =
          client->GetAccountPasswordStore()) {
    // base::Unretained() safe because of the precondition.
    return account_store->AddSyncEnabledOrDisabledCallback(base::BindRepeating(
        &PasswordManager::UpdateFormManagers, base::Unretained(manager)));
  }
  return {};
}

}  // namespace

// static
void PasswordManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kCredentialsEnableService, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
#if BUILDFLAG(IS_IOS)
  registry->RegisterBooleanPref(prefs::kCredentialProviderEnabledOnStartup,
                                false);
#endif  // BUILDFLAG(IS_IOS)
  registry->RegisterBooleanPref(
      prefs::kCredentialsEnableAutosignin, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterStringPref(prefs::kSyncPasswordHash, std::string(),
                               PrefRegistry::NO_REGISTRATION_FLAGS);
  registry->RegisterStringPref(prefs::kSyncPasswordLengthAndHashSalt,
                               std::string(),
                               PrefRegistry::NO_REGISTRATION_FLAGS);
  registry->RegisterBooleanPref(
      prefs::kWasAutoSignInFirstRunExperienceShown, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);
  registry->RegisterDoublePref(prefs::kLastTimeObsoleteHttpCredentialsRemoved,
                               0.0);
  registry->RegisterDoublePref(prefs::kLastTimePasswordCheckCompleted, 0.0);
  registry->RegisterDoublePref(prefs::kLastTimePasswordStoreMetricsReported,
                               0.0);
  registry->RegisterTimePref(
      prefs::kSyncedLastTimePasswordCheckCompleted, base::Time(),
      user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF);

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  registry->RegisterDictionaryPref(prefs::kAccountStoragePerAccountSettings);
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)

  registry->RegisterTimePref(prefs::kProfileStoreDateLastUsedForFilling,
                             base::Time());
  registry->RegisterTimePref(prefs::kAccountStoreDateLastUsedForFilling,
                             base::Time());
  registry->RegisterBooleanPref(prefs::kWereOldGoogleLoginsRemoved, false);

#if BUILDFLAG(IS_APPLE)
  registry->RegisterIntegerPref(prefs::kKeychainMigrationStatus,
                                4 /* MIGRATED_DELETED */);
#endif  // BUILDFLAG(IS_APPLE)
  registry->RegisterListPref(prefs::kPasswordHashDataList,
                             PrefRegistry::NO_REGISTRATION_FLAGS);
  registry->RegisterBooleanPref(
      prefs::kPasswordLeakDetectionEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kPasswordDismissCompromisedAlertEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kPasswordsPrefWithNewLabelUsed, false);
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kOfferToSavePasswordsEnabledGMS, true);
  registry->RegisterBooleanPref(prefs::kSavePasswordsSuspendedByError, false);
  registry->RegisterBooleanPref(prefs::kAutoSignInEnabledGMS, true);
  registry->RegisterBooleanPref(prefs::kSettingsMigratedToUPMLocal, false);
  registry->RegisterIntegerPref(
      prefs::kCurrentMigrationVersionToGoogleMobileServices, 0);
  registry->RegisterDoublePref(prefs::kTimeOfLastMigrationAttempt, 0.0);
  registry->RegisterBooleanPref(prefs::kPasswordsUseUPMLocalAndSeparateStores,
                                false);
  registry->RegisterBooleanPref(prefs::kRequiresMigrationAfterSyncStatusChange,
                                false);
  registry->RegisterBooleanPref(
      prefs::kUnenrolledFromGoogleMobileServicesDueToErrors, false);
  registry->RegisterIntegerPref(
      prefs::kUnenrolledFromGoogleMobileServicesAfterApiErrorCode, 0);
  registry->RegisterIntegerPref(
      prefs::kUnenrolledFromGoogleMobileServicesWithErrorListVersion, 0);
  registry->RegisterStringPref(prefs::kUPMErrorUIShownTimestamp, "0");
  registry->RegisterIntegerPref(prefs::kTimesReenrolledToGoogleMobileServices,
                                0);
  registry->RegisterIntegerPref(
      prefs::kTimesAttemptedToReenrollToGoogleMobileServices, 0);
  registry->RegisterBooleanPref(
      prefs::kUserAcknowledgedLocalPasswordsMigrationWarning, false);
  registry->RegisterTimePref(
      prefs::kLocalPasswordsMigrationWarningShownTimestamp, base::Time());
  registry->RegisterBooleanPref(
      prefs::kLocalPasswordMigrationWarningShownAtStartup, false);
  registry->RegisterIntegerPref(
      prefs::kLocalPasswordMigrationWarningPrefsVersion, 0);
  registry->RegisterIntegerPref(
      prefs::kPasswordGenerationBottomSheetDismissCount, 0);
#endif  // BUILDFLAG(IS_ANDROID)
  // Preferences for |PasswordChangeSuccessTracker|.
  registry->RegisterIntegerPref(prefs::kPasswordChangeSuccessTrackerVersion, 0);
  registry->RegisterListPref(prefs::kPasswordChangeSuccessTrackerFlows);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterIntegerPref(
      prefs::kBiometricAuthBeforeFillingPromoShownCounter, 0);
  registry->RegisterBooleanPref(prefs::kHasUserInteractedWithBiometricAuthPromo,
                                false);
  registry->RegisterBooleanPref(prefs::kBiometricAuthenticationBeforeFilling,
                                false);
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
#if BUILDFLAG(IS_IOS)
  registry->RegisterBooleanPref(prefs::kAccountStorageNoticeShown, false);
  registry->RegisterIntegerPref(prefs::kAccountStorageNewFeatureIconImpressions,
                                0);
#endif  // BUILDFLAG(IS_IOS)
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)  // Desktop
  registry->RegisterListPref(prefs::kPasswordManagerPromoCardsList);
#endif  // BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  registry->RegisterBooleanPref(prefs::kPasswordSharingEnabled, true);
}

// static
void PasswordManager::RegisterLocalPrefs(PrefRegistrySimple* registry) {
#if BUILDFLAG(IS_WIN)
  registry->RegisterInt64Pref(prefs::kOsPasswordLastChanged, 0);
  registry->RegisterBooleanPref(prefs::kOsPasswordBlank, false);
  registry->RegisterBooleanPref(prefs::kIsBiometricAvailable, false);
#endif  // BUILDFLAG(IS_WIN)
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kHadBiometricsAvailable, false);
#endif  // BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
}

PasswordManager::PasswordManager(PasswordManagerClient* client)
    : client_(client),
      account_store_cb_list_subscription_(
          AddSyncEnabledOrDisabledCallback(this, client)),
      leak_delegate_(client) {
  DCHECK(client_);
}

PasswordManager::~PasswordManager() = default;

void PasswordManager::OnGeneratedPasswordAccepted(
    PasswordManagerDriver* driver,
    const FormData& form_data,
    autofill::FieldRendererId generation_element_id,
    const std::u16string& password) {
  PasswordFormManager* manager =
      GetMatchedManager(driver, form_data.unique_renderer_id);
  if (manager) {
    manager->OnGeneratedPasswordAccepted(form_data, generation_element_id,
                                         password);
  } else {
    // OnPresaveGeneratedPassword records the histogram in all other cases.
    UMA_HISTOGRAM_BOOLEAN("PasswordManager.GeneratedFormHasNoFormManager",
                          true);
  }
}

void PasswordManager::OnPasswordNoLongerGenerated(PasswordManagerDriver* driver,
                                                  const FormData& form_data) {
  DCHECK(client_->IsSavingAndFillingEnabled(form_data.url));

  PasswordFormManager* form_manager =
      GetMatchedManager(driver, form_data.unique_renderer_id);
  if (form_manager)
    form_manager->PasswordNoLongerGenerated();
}

void PasswordManager::SetGenerationElementAndTypeForForm(
    password_manager::PasswordManagerDriver* driver,
    FormRendererId form_id,
    FieldRendererId generation_element,
    autofill::password_generation::PasswordGenerationType type) {
  PasswordFormManager* form_manager = GetMatchedManager(driver, form_id);
  if (form_manager) {
    DCHECK(client_->IsSavingAndFillingEnabled(form_manager->GetURL()));
    form_manager->SetGenerationElement(generation_element);
    form_manager->SetGenerationPopupWasShown(type);
  }
}

void PasswordManager::OnPresaveGeneratedPassword(
    PasswordManagerDriver* driver,
    const FormData& form_data,
    const std::u16string& generated_password) {
  DCHECK(client_->IsSavingAndFillingEnabled(form_data.url));
  PasswordFormManager* form_manager =
      GetMatchedManager(driver, form_data.unique_renderer_id);
  UMA_HISTOGRAM_BOOLEAN("PasswordManager.GeneratedFormHasNoFormManager",
                        !form_manager);
  if (form_manager) {
    form_manager->PresaveGeneratedPassword(form_data, generated_password);
#if BUILDFLAG(IS_IOS)
    // On iOS some field values are not propagated to PasswordManager timely.
    // Provisionally save entire |form_data| to make sure the form is parsed
    // properly afterwards (crbug.com/1170351).
    // TODO(crbug/1399524): Invoke this from SharedPasswordController.
    form_manager->ProvisionallySave(form_data, driver, &possible_usernames_);
#endif  // BUILDFLAG(IS_IOS)
  }
}

PasswordManagerClient* PasswordManager::GetClient() {
  return client_;
}

void PasswordManager::DidNavigateMainFrame(bool form_may_be_submitted) {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogBoolean(Logger::STRING_DID_NAVIGATE_MAIN_FRAME,
                       form_may_be_submitted);
  }

  if (client_->IsNewTabPage()) {
    if (logger)
      logger->LogMessage(Logger::STRING_NAVIGATION_NTP);
    // On a successful Chrome sign-in the page navigates to the new tab page
    // (ntp). OnPasswordFormsRendered is not called on ntp. That is
    // why the standard flow for saving hash does not work. Save a password hash
    // now since a navigation to ntp is the sign of successful sign-in.
    PasswordFormManager* manager = GetSubmittedManager();
    if (manager && manager->GetSubmittedForm()
                       ->form_data.is_gaia_with_skip_save_password_form) {
      MaybeSavePasswordHash(manager);
    }
  }

  // Reset |possible_username_| if the navigation cannot be a result of form
  // submission.
  if (!form_may_be_submitted)
    possible_usernames_.Clear();

  for (std::unique_ptr<PasswordFormManager>& manager : form_managers_) {
    if (form_may_be_submitted && manager->is_submitted()) {
      owned_submitted_form_manager_ = std::move(manager);
      break;
    }
  }
  form_managers_.clear();

  TryToFindPredictionsToPossibleUsernames();
  predictions_.clear();
  store_password_called_ = false;
}

void PasswordManager::UpdateFormManagers() {
  // Get the fetchers and all the drivers.
  std::vector<FormFetcher*> fetchers;
  std::vector<PasswordManagerDriver*> drivers;
  for (const auto& form_manager : form_managers_) {
    fetchers.push_back(form_manager->GetFormFetcher());
    if (form_manager->GetDriver())
      drivers.push_back(form_manager->GetDriver().get());
  }

  // Remove the duplicates.
  base::ranges::sort(fetchers);
  fetchers.erase(base::ranges::unique(fetchers), fetchers.end());
  base::ranges::sort(drivers);
  drivers.erase(base::ranges::unique(drivers), drivers.end());
  // Refetch credentials for all the forms and update the drivers.
  for (FormFetcher* fetcher : fetchers)
    fetcher->Fetch();

  // The autofill manager will be repopulated again when the credentials
  // are retrieved.
  for (PasswordManagerDriver* driver : drivers) {
    // GetPasswordAutofillManager() is returning nullptr in iOS Chrome, since
    // PasswordAutofillManager is not instantiated on iOS Chrome.
    // See //components/password_manager/ios/ios_password_manager_driver.mm
    if (driver->GetPasswordAutofillManager()) {
      driver->GetPasswordAutofillManager()->DeleteFillData();
    }
  }
}

void PasswordManager::DropFormManagers() {
  form_managers_.clear();
  owned_submitted_form_manager_.reset();
  visible_forms_data_.clear();
  TryToFindPredictionsToPossibleUsernames();
  predictions_.clear();
}

const std::vector<const PasswordForm*>* PasswordManager::GetBestMatches(
    PasswordManagerDriver* driver,
    autofill::FormRendererId form_id) {
  PasswordFormManager* manager = GetMatchedManager(driver, form_id);
  return manager ? &(manager->GetBestMatches()) : nullptr;
}

bool PasswordManager::IsPasswordFieldDetectedOnPage() const {
  return !form_managers_.empty();
}

void PasswordManager::OnPasswordFormSubmitted(PasswordManagerDriver* driver,
                                              const FormData& form_data) {
  ProvisionallySaveForm(form_data, driver, false);
}

void PasswordManager::OnDynamicFormSubmission(
    password_manager::PasswordManagerDriver* driver,
    SubmissionIndicatorEvent event) {
  if (password_manager_util::IsLoggingActive(client_)) {
    BrowserSavePasswordProgressLogger logger(client_->GetLogManager());
    logger.LogMessage(Logger::STRING_ON_DYNAMIC_FORM_SUBMISSION);
  }
  PasswordFormManager* submitted_manager = GetSubmittedManager();
  // TODO(crbug.com/949519): Add UMA metric for how frequently submitted_manager
  // is actually null.
  if (!submitted_manager || !submitted_manager->GetSubmittedForm())
    return;

  const PasswordForm* submitted_form = submitted_manager->GetSubmittedForm();

  const GURL gaia_signon_realm =
      GaiaUrls::GetInstance()->gaia_origin().GetURL();
  if (GURL(submitted_form->signon_realm) == gaia_signon_realm) {
    // The GAIA signon realm (i.e. https://accounts.google.com) will always
    // perform a full page redirect once the user cleared the login flow. Thus
    // don't respond to other JavaScript based signals that would result in
    // false positives with regard to successful logins.
    return;
  }

  submitted_manager->UpdateSubmissionIndicatorEvent(event);

  if (IsAutomaticSavePromptAvailable())
    OnLoginSuccessful();
}

void PasswordManager::OnPasswordFormCleared(
    PasswordManagerDriver* driver,
    const autofill::FormData& form_data) {
  if (password_manager_util::IsLoggingActive(client_)) {
    BrowserSavePasswordProgressLogger logger(client_->GetLogManager());
    logger.LogMessage(Logger::STRING_ON_PASSWORD_FORM_CLEARED);
  }
  PasswordFormManager* manager =
      GetMatchedManager(driver, form_data.unique_renderer_id);
  if (!manager || !IsAutomaticSavePromptAvailable(manager) ||
      !manager->HasLikelyChangeOrResetFormSubmitted()) {
    return;
  }
  // If a password form was cleared, login is successful.
  if (form_data.is_form_tag) {
    manager->UpdateSubmissionIndicatorEvent(
        SubmissionIndicatorEvent::CHANGE_PASSWORD_FORM_CLEARED);
    OnLoginSuccessful();
    return;
  }
  // If password fields outside the <form> tag were cleared, it should be
  // verified that fields are relevant.
  FieldRendererId new_password_field_id =
      manager->GetSubmittedForm()->new_password_element_renderer_id;
  auto it = base::ranges::find(form_data.fields, new_password_field_id,
                               &autofill::FormFieldData::unique_renderer_id);
  if (it != form_data.fields.end() && it->value.empty()) {
    manager->UpdateSubmissionIndicatorEvent(
        SubmissionIndicatorEvent::CHANGE_PASSWORD_FORM_CLEARED);
    OnLoginSuccessful();
  }
}

#if BUILDFLAG(IS_IOS)
void PasswordManager::OnSubframeFormSubmission(PasswordManagerDriver* driver,
                                               const FormData& form_data) {
  if (password_manager_util::IsLoggingActive(client_)) {
    BrowserSavePasswordProgressLogger logger(client_->GetLogManager());
    logger.LogMessage(Logger::STRING_ON_DYNAMIC_FORM_SUBMISSION);
  }

  ProvisionallySaveForm(form_data, driver, false);

  if (IsAutomaticSavePromptAvailable())
    OnLoginSuccessful();
}
#endif  // BUILDFLAG(IS_IOS)

void PasswordManager::OnUserModifiedNonPasswordField(
    PasswordManagerDriver* driver,
    autofill::FieldRendererId renderer_id,
    const std::u16string& value,
    bool autocomplete_attribute_has_username,
    bool is_likely_otp) {
  // |driver| might be empty on iOS or in tests.
  int driver_id = driver ? driver->GetId() : 0;
  possible_usernames_.Put(
      PossibleUsernameFieldIdentifier{driver_id, renderer_id},
      PossibleUsernameData(GetSignonRealm(driver->GetLastCommittedURL()),
                           renderer_id, value, base::Time::Now(), driver_id,
                           autocomplete_attribute_has_username, is_likely_otp));

  if (base::FeatureList::IsEnabled(
          password_manager::features::kForgotPasswordFormSupport)) {
    FieldInfoManager* field_info_manager = client_->GetFieldInfoManager();
    // The manager might not exist in incognito.
    if (!field_info_manager) {
      return;
    }
    field_info_manager->AddFieldInfo(
        {driver_id, renderer_id, GetSignonRealm(driver->GetLastCommittedURL()),
         value, is_likely_otp},
        FindPredictionsForField(renderer_id, driver_id));
  }
}

void PasswordManager::OnInformAboutUserInput(PasswordManagerDriver* driver,
                                             const FormData& form_data) {
  PasswordFormManager* manager = ProvisionallySaveForm(form_data, driver, true);

  auto availability =
      manager ? PasswordManagerMetricsRecorder::FormManagerAvailable::kSuccess
              : PasswordManagerMetricsRecorder::FormManagerAvailable::
                    kMissingManual;
  if (client_->GetMetricsRecorder())
    client_->GetMetricsRecorder()->RecordFormManagerAvailable(availability);

  ShowManualFallbackForSaving(manager, form_data);
}

void PasswordManager::HideManualFallbackForSaving() {
  client_->HideManualFallbackForSaving();
}

bool PasswordManager::HaveFormManagersReceivedData(
    const PasswordManagerDriver* driver) {
  // If no form managers exist to have requested logins, no data was received
  // either.
  if (form_managers_.empty())
    return false;
  for (const auto& form_manager : form_managers_) {
    if (form_manager->GetDriver().get() == driver &&
        form_manager->GetFormFetcher()->GetState() ==
            FormFetcher::State::WAITING) {
      return false;
    }
  }
  return true;
}

void PasswordManager::OnPasswordFormsParsed(
    PasswordManagerDriver* driver,
    const std::vector<FormData>& form_data) {
  if (NewFormsParsed(driver, form_data))
    client_->RefreshPasswordManagerSettingsIfNeeded();
  CreatePendingLoginManagers(driver, form_data);

  PasswordGenerationFrameHelper* password_generation_manager =
      driver ? driver->GetPasswordGenerationHelper() : nullptr;
  if (password_generation_manager) {
    password_generation_manager->PrefetchSpec(
        client_->GetLastCommittedOrigin().GetURL());
  }
}

void PasswordManager::CreatePendingLoginManagers(
    PasswordManagerDriver* driver,
    const std::vector<FormData>& forms_data) {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogMessage(Logger::STRING_CREATE_LOGIN_MANAGERS_METHOD);
  }

  CreateFormManagers(driver, forms_data);

  // Record whether or not this top-level URL has at least one password field.
  client_->AnnotateNavigationEntry(!forms_data.empty());
}

void PasswordManager::CreateFormManagers(
    PasswordManagerDriver* driver,
    const std::vector<FormData>& forms_data) {
  // Find new forms.
  std::vector<const FormData*> new_forms_data;
  for (const FormData& form_data : forms_data) {
    if (!client_->IsFillingEnabled(form_data.url))
      continue;

    PasswordFormManager* manager =
        GetMatchedManager(driver, form_data.unique_renderer_id);
    if (!manager) {
      new_forms_data.push_back(&form_data);
      continue;
    }

    if (HasObservedFormChanged(form_data, *manager)) {
      manager->UpdateFormManagerWithFormChanges(form_data, predictions_);
    }
    // Call fill regardless of whether the form was updated. In the no-update
    // case, this call provides extra redundancy to handle cases in which the
    // site overrides filled values.
    manager->Fill();
  }

  // Create form manager for new forms.
  for (const FormData* new_form_data : new_forms_data)
    CreateFormManager(driver, *new_form_data);
}

PasswordFormManager* PasswordManager::CreateFormManager(
    PasswordManagerDriver* driver,
    const autofill::FormData& form) {
  form_managers_.push_back(std::make_unique<PasswordFormManager>(
      client_,
      driver ? driver->AsWeakPtr() : base::WeakPtr<PasswordManagerDriver>(),
      form, nullptr, std::make_unique<PasswordSaveManagerImpl>(client_),
      nullptr));
  form_managers_.back()->ProcessServerPredictions(predictions_);
  return form_managers_.back().get();
}

PasswordFormManager* PasswordManager::ProvisionallySaveForm(
    const FormData& submitted_form,
    PasswordManagerDriver* driver,
    bool is_manual_fallback) {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogMessage(Logger::STRING_PROVISIONALLY_SAVE_FORM_METHOD);
  }
  if (!client_->IsSavingAndFillingEnabled(submitted_form.url)) {
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::SAVING_DISABLED, submitted_form.url);
    return nullptr;
  }

  if (store_password_called_)
    return nullptr;

  const GURL& submitted_url = submitted_form.url;
  if (ShouldBlockPasswordForSameOriginButDifferentScheme(submitted_url)) {
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::SAVING_ON_HTTP_AFTER_HTTPS,
        submitted_url);
    return nullptr;
  }

  PasswordFormManager* matched_manager =
      GetMatchedManager(driver, submitted_form.unique_renderer_id);

  auto availability =
      matched_manager
          ? PasswordManagerMetricsRecorder::FormManagerAvailable::kSuccess
          : PasswordManagerMetricsRecorder::FormManagerAvailable::
                kMissingProvisionallySave;
  if (client_->GetMetricsRecorder())
    client_->GetMetricsRecorder()->RecordFormManagerAvailable(availability);

  if (!matched_manager) {
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::NO_MATCHING_FORM, submitted_form.url);
    matched_manager = CreateFormManager(driver, submitted_form);
  }

  if (is_manual_fallback && matched_manager->GetFormFetcher()->GetState() ==
                                FormFetcher::State::WAITING) {
    // In case of manual fallback, the form manager has to be ready for saving.
    return nullptr;
  }

  TryToFindPredictionsToPossibleUsernames();
  if (!matched_manager->ProvisionallySave(submitted_form, driver,
                                          &possible_usernames_)) {
    return nullptr;
  }

  // |matched_manager->ProvisionallySave| returning true means that there is a
  // nonempty password field. If such |matched_manager| contains any of
  // possible usernames, clear it from single username candidates.
  for (auto possible_username_iterator = possible_usernames_.begin();
       possible_username_iterator != possible_usernames_.end();) {
    if (matched_manager->ObservedFormHasField(
            possible_username_iterator->first.driver_id,
            possible_username_iterator->first.renderer_id)) {
      possible_username_iterator =
          possible_usernames_.Erase(possible_username_iterator);
    } else {
      ++possible_username_iterator;
    }
  }

  // Set all other form managers to no submission state.
  for (const auto& manager : form_managers_) {
    if (manager.get() != matched_manager)
      manager->set_not_submitted();
  }

  // Cache the committed URL. Once the post-submit navigation concludes, we
  // compare the landing URL against the cached and report the difference.
  submitted_form_url_ = submitted_url;

  return matched_manager;
}

#if BUILDFLAG(USE_BLINK)
void PasswordManager::LogFirstFillingResult(
    PasswordManagerDriver* driver,
    autofill::FormRendererId form_renderer_id,
    int32_t result) {
  PasswordFormManager* matching_manager =
      FindMatchedManagerByRendererId(form_renderer_id, form_managers_, driver);
  if (!matching_manager)
    return;
  matching_manager->GetMetricsRecorder()->RecordFirstFillingResult(result);
}
#endif  // BUILDFLAG(USE_BLINK)

void PasswordManager::NotifyStorePasswordCalled() {
  store_password_called_ = true;
  DropFormManagers();
}

#if BUILDFLAG(IS_IOS)
void PasswordManager::UpdateStateOnUserInput(
    PasswordManagerDriver* driver,
    FormRendererId form_id,
    FieldRendererId field_id,
    const std::u16string& field_value) {
  PasswordFormManager* manager = GetMatchedManager(driver, form_id);
  if (!manager)
    return;

  manager->UpdateStateOnUserInput(form_id, field_id, field_value);
  OnInformAboutUserInput(driver, *manager->observed_form());
}

void PasswordManager::OnPasswordNoLongerGenerated(
    PasswordManagerDriver* driver) {
  for (std::unique_ptr<PasswordFormManager>& manager : form_managers_)
    manager->PasswordNoLongerGenerated();
}

void PasswordManager::OnPasswordFormRemoved(
    PasswordManagerDriver* driver,
    const FieldDataManager& field_data_manager,
    FormRendererId form_id) {
  for (auto& manager : form_managers_) {
    // Find a form with corresponding renderer id.
    if (manager->DoesManage(form_id, driver)) {
      DetectPotentialSubmission(manager.get(), field_data_manager, driver);
      return;
    }
  }
}

void PasswordManager::OnIframeDetach(
    const std::string& frame_id,
    PasswordManagerDriver* driver,
    const FieldDataManager& field_data_manager) {
  for (auto& manager : form_managers_) {
    // Find a form with corresponding frame id. Stop iterating in case the
    // target form manager was found to avoid crbug.com/1129758 and since only
    // one password form is being submitted at a time.
    if (manager->observed_form()->frame_id == frame_id &&
        DetectPotentialSubmission(manager.get(), field_data_manager, driver)) {
      return;
    }
  }
}

void PasswordManager::PropagateFieldDataManagerInfo(
    const FieldDataManager& field_data_manager,
    const PasswordManagerDriver* driver) {
  for (auto& manager : form_managers_) {
    // The current method can be called with the same driver for different
    // forms. If the forms are in different frames, then only some of them will
    // match the driver, since each frame has its own driver. Thus, we return
    // early if the driver doesn't match the frame of the form.
    if (manager->GetDriver().get() != driver) {
      continue;
    }
    if (!client_->IsSavingAndFillingEnabled(manager->GetURL())) {
      RecordProvisionalSaveFailure(
          PasswordManagerMetricsRecorder::SAVING_DISABLED, manager->GetURL());
      continue;
    }
    manager->ProvisionallySaveFieldDataManagerInfo(field_data_manager, driver);
  }
}
#endif  // BUILDFLAG(IS_IOS)

bool PasswordManager::IsAutomaticSavePromptAvailable(
    PasswordFormManager* form_manager) {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogMessage(Logger::STRING_CAN_PROVISIONAL_MANAGER_SAVE_METHOD);
  }

  PasswordFormManager* submitted_manager = GetSubmittedManager();

  if (!submitted_manager) {
    if (logger) {
      logger->LogMessage(Logger::STRING_NO_PROVISIONAL_SAVE_MANAGER);
    }
    return false;
  }

  // As OnLoginSuccessful always works with the result of GetSubmittedManager(),
  // consider only that form manager when a specific |form_manager| is provided.
  if (form_manager && (form_manager != submitted_manager)) {
    if (logger) {
      logger->LogMessage(Logger::STRING_ANOTHER_MANAGER_WAS_SUBMITTED);
    }
    return false;
  }

  if (submitted_manager->GetFormFetcher()->GetState() ==
      FormFetcher::State::WAITING) {
    // We have a provisional save manager, but it didn't finish matching yet.
    // We just give up.
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::MATCHING_NOT_COMPLETE,
        submitted_manager->GetURL());
    return false;
  }

  return !submitted_manager->GetPendingCredentials().only_for_fallback;
}

bool PasswordManager::ShouldBlockPasswordForSameOriginButDifferentScheme(
    const GURL& url) const {
  return submitted_form_url_.host_piece() == url.host_piece() &&
         submitted_form_url_.SchemeIsCryptographic() &&
         !url.SchemeIsCryptographic();
}

void PasswordManager::OnPasswordFormsRendered(
    password_manager::PasswordManagerDriver* driver,
    const std::vector<FormData>& visible_forms_data) {
  CreatePendingLoginManagers(driver, visible_forms_data);
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogMessage(Logger::STRING_ON_PASSWORD_FORMS_RENDERED_METHOD);
  }

  // No submitted manager => no submission tracking.
  if (!GetSubmittedManager())
    client_->ResetSubmissionTrackingAfterTouchToFill();

  if (!IsAutomaticSavePromptAvailable())
    return;

  PasswordFormManager* submitted_manager = GetSubmittedManager();

  // If the server throws an internal error, access denied page, page not
  // found etc. after a login attempt, we do not save the credentials.
  if (client_->WasLastNavigationHTTPError()) {
    if (logger)
      logger->LogMessage(Logger::STRING_DECISION_DROP);
    submitted_manager->GetMetricsRecorder()->LogSubmitFailed();
    ResetSubmittedManager();
    return;
  }

  if (logger) {
    logger->LogNumber(Logger::STRING_NUMBER_OF_VISIBLE_FORMS,
                      visible_forms_data.size());
  }

  // Record all visible forms from the frame.
  visible_forms_data_.insert(visible_forms_data_.end(),
                             visible_forms_data.begin(),
                             visible_forms_data.end());
  if (
#if BUILDFLAG(IS_IOS)
      // On iOS, drivers are bound to WebFrames, but some pages (e.g. files)
      // do not lead to creating WebFrame objects, therefore. If the driver is
      // missing, the current page has no password forms, but we still are
      // interested in detecting a submisison.
      driver &&
#endif  // BUILDFLAG(IS_IOS)
      !driver->IsInPrimaryMainFrame() &&
      submitted_manager->GetFrameId() != driver->GetFrameId()) {
    // Frames different from the main frame and the frame of the submitted form
    // are unlikely relevant to success of submission.
    return;
  }

  // If we see the login form again, then the login failed.
  if (submitted_manager->GetPendingCredentials().scheme ==
      PasswordForm::Scheme::kHtml) {
    for (const FormData& form_data : visible_forms_data_) {
      if (submitted_manager->IsEqualToSubmittedForm(form_data)) {
        if (submitted_manager->HasLikelyChangeOrResetFormSubmitted() &&
            AreChangePasswordFieldsEmpty(
                form_data, *submitted_manager->GetSubmittedForm())) {
          continue;
        }
        submitted_manager->GetMetricsRecorder()->LogSubmitFailed();
        if (logger) {
          logger->LogFormData(Logger::STRING_PASSWORD_FORM_REAPPEARED,
                              form_data);
          logger->LogMessage(Logger::STRING_DECISION_DROP);
        }
        ResetSubmittedManager();
        // Clear visible_forms_data_ once we found the match.
        visible_forms_data_.clear();
        return;
      }
    }
  } else {
    if (logger)
      logger->LogMessage(Logger::STRING_PROVISIONALLY_SAVED_FORM_IS_NOT_HTML);
  }
  // Clear visible_forms_data_ after checking all the visible forms.
  visible_forms_data_.clear();

  // Looks like a successful login attempt. Either show an infobar or
  // automatically save the login data. We prompt when the user hasn't
  // already given consent, either through previously accepting the infobar
  // or by having the browser generate the password.
  OnLoginSuccessful();
}

void PasswordManager::OnLoginSuccessful() {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogMessage(Logger::STRING_ON_ASK_USER_OR_SAVE_PASSWORD);
  }

  PasswordFormManager* submitted_manager = GetSubmittedManager();
  CHECK(submitted_manager);
  const PasswordForm* submitted_form = submitted_manager->GetSubmittedForm();
  CHECK(submitted_form);

  // User might fill several login flows during their user journey. For example,
  // Forgot Password Flow followed by sign-in flow. To not suggest usernames
  // from the old flow, clear after successful login.
  possible_usernames_.Clear();

  client_->MaybeReportEnterpriseLoginEvent(
      submitted_form->url, submitted_form->IsFederatedCredential(),
      submitted_form->federation_origin,
      submitted_manager->GetPendingCredentials().username_value);
  client_->NotifyOnSuccessfulLogin(submitted_form->username_value);
  if (!client_->IsSavingAndFillingEnabled(submitted_form->url))
    return;

  client_->GetStoreResultFilter()->ReportFormLoginSuccess(*submitted_manager);
  // Check for leaks only if there are no muted credentials and it is not a
  // single username submission (a leak warning may offer an automated password
  // change, which requires a user to be logged in).
  if (!HasMutedCredentials(
          submitted_manager->GetInsecureCredentials(),
          submitted_manager->GetSubmittedForm()->username_value) &&
      !IsSingleUsernameSubmission(*submitted_manager->GetSubmittedForm())) {
    leak_delegate_.StartLeakCheck(LeakDetectionInitiator::kSignInCheck,
                                  submitted_manager->GetPendingCredentials());
  }

  auto submission_event =
      submitted_manager->GetSubmittedForm()->submission_event;
  metrics_util::LogPasswordSuccessfulSubmissionIndicatorEvent(submission_event);
  if (logger)
    logger->LogSuccessfulSubmissionIndicatorEvent(submission_event);

  bool able_to_save_passwords =
      client_->GetProfilePasswordStore()->IsAbleToSavePasswords();
  UMA_HISTOGRAM_BOOLEAN("PasswordManager.AbleToSavePasswordsOnSuccessfulLogin",
                        able_to_save_passwords);
  if (!able_to_save_passwords)
    return;

  MaybeSavePasswordHash(submitted_manager);

  // TODO(https://crbug.com/831123): Implement checking whether to save with
  // PasswordFormManager.
  // Check whether the filter allows saving this credential. In practice, this
  // prevents saving the password of the syncing account. However, if the
  // password is already saved, then *updating* it is still allowed - better
  // than keeping an outdated password around.
  if (!submitted_manager->IsPasswordUpdate() &&
      !client_->GetStoreResultFilter()->ShouldSave(
          *submitted_manager->GetSubmittedForm())) {
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::SYNC_CREDENTIAL,
        submitted_manager->GetURL());
    ResetSubmittedManager();
    return;
  }

  submitted_manager->GetMetricsRecorder()->LogSubmitPassed();

  UMA_HISTOGRAM_BOOLEAN(
      "PasswordManager.SuccessfulLoginHappened",
      submitted_manager->GetSubmittedForm()->url.SchemeIsCryptographic());

  // If the form is eligible only for saving fallback, it shouldn't go here.
  CHECK(!submitted_manager->GetPendingCredentials().only_for_fallback);

  if (ShouldPromptUserToSavePassword(*submitted_manager)) {
    if (logger)
      logger->LogMessage(Logger::STRING_DECISION_ASK);
    submitted_manager->SaveSuggestedUsernameValueToVotesUploader();
    bool update_password = submitted_manager->IsPasswordUpdate();
    if (client_->PromptUserToSaveOrUpdatePassword(MoveOwnedSubmittedManager(),
                                                  update_password)) {
      if (logger)
        logger->LogMessage(Logger::STRING_SHOW_PASSWORD_PROMPT);
    }
  } else {
    if (logger)
      logger->LogMessage(Logger::STRING_DECISION_SAVE);
    submitted_manager->Save();

    if (!submitted_manager->IsNewLogin()) {
      client_->NotifySuccessfulLoginWithExistingPassword(
          submitted_manager->Clone());
    }

    if (submitted_manager->HasGeneratedPassword()) {
      client_->AutomaticPasswordSave(MoveOwnedSubmittedManager(),
                                     /*is_update_confirmation=*/false);
    }
  }
  ResetSubmittedManager();
}

void PasswordManager::MaybeSavePasswordHash(
    PasswordFormManager* submitted_manager) {
  if (!base::FeatureList::IsEnabled(features::kPasswordReuseDetectionEnabled)) {
    return;
  }

  const PasswordForm* submitted_form = submitted_manager->GetSubmittedForm();
  // When |username_value| is empty, it's not clear whether the submitted
  // credentials are really Gaia or enterprise credentials. Don't save
  // password hash in that case.
  std::string username = base::UTF16ToUTF8(submitted_form->username_value);
  if (username.empty())
    return;

  password_manager::PasswordReuseManager* reuse_manager =
      client_->GetPasswordReuseManager();
  // May be null in tests.
  if (!reuse_manager)
    return;

  bool should_save_enterprise_pw =
      client_->GetStoreResultFilter()->ShouldSaveEnterprisePasswordHash(
          *submitted_form);
  bool should_save_gaia_pw =
      client_->GetStoreResultFilter()->ShouldSaveGaiaPasswordHash(
          *submitted_form);

  if (!should_save_enterprise_pw && !should_save_gaia_pw)
    return;

  if (password_manager_util::IsLoggingActive(client_)) {
    BrowserSavePasswordProgressLogger logger(client_->GetLogManager());
    logger.LogMessage(Logger::STRING_SAVE_PASSWORD_HASH);
  }

  // Canonicalizes username if it is an email.
  if (username.find('@') != std::string::npos)
    username = gaia::CanonicalizeEmail(username);
  bool is_password_change = !submitted_form->new_password_element.empty();
  const std::u16string password = is_password_change
                                      ? submitted_form->new_password_value
                                      : submitted_form->password_value;

  if (should_save_enterprise_pw) {
    reuse_manager->SaveEnterprisePasswordHash(username, password);
    return;
  }

  DCHECK(should_save_gaia_pw);
  bool is_sync_account_email =
      client_->GetStoreResultFilter()->IsSyncAccountEmail(username);
  metrics_util::GaiaPasswordHashChange event =
      is_sync_account_email
          ? (is_password_change
                 ? metrics_util::GaiaPasswordHashChange::CHANGED_IN_CONTENT_AREA
                 : metrics_util::GaiaPasswordHashChange::SAVED_IN_CONTENT_AREA)
          : (is_password_change
                 ? metrics_util::GaiaPasswordHashChange::
                       NOT_SYNC_PASSWORD_CHANGE
                 : metrics_util::GaiaPasswordHashChange::SAVED_IN_CONTENT_AREA);
  reuse_manager->SaveGaiaPasswordHash(
      username, password,
      /*is_sync_password_for_metrics=*/is_sync_account_email, event);
}

void PasswordManager::ProcessAutofillPredictions(
    PasswordManagerDriver* driver,
    const autofill::FormData& form,
    const base::flat_map<autofill::FieldGlobalId,
                         autofill::AutofillType::ServerPrediction>&
        predictions) {
  // Don't do anything if Password store is not available.
  if(!client_->GetProfilePasswordStore())
    return;

  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
    logger->LogFormDataWithServerPredictions(Logger::STRING_SERVER_PREDICTIONS,
                                             form, predictions);
  }

  // `driver` might be null in tests.
  int driver_id = driver ? driver->GetId() : 0;
  // Update the `predictions_` stored as a member.
  const FormPredictions& form_predictions =
      predictions_
          .insert_or_assign(
              CalculateFormSignature(form),
              ConvertToFormPredictions(driver_id, form, predictions))
          .first->second;

  if (PasswordGenerationFrameHelper* password_generation_manager =
          driver ? driver->GetPasswordGenerationHelper() : nullptr) {
    password_generation_manager->ProcessPasswordRequirements(form, predictions);
  }

  // Process predictions in case they arrived after the user interacted with
  // potential username fields. The manager might not exist in incognito.
  if (FieldInfoManager* field_info_manager = client_->GetFieldInfoManager()) {
    field_info_manager->ProcessServerPredictions(predictions_);
  }

  // Create or update the `PasswordFormManager` corresponding to `form`.
  PasswordFormManager* manager =
      GetMatchedManager(driver, form.global_id().renderer_id);
  if (!manager) {
    // If the renderer recognizes `form` as a credential form, then we will
    // be informed about this form via `OnFormsParsed()` and `OnFormsSeen()`.
    if (!IsRelevantForPasswordManagerButNotRecognizedByRenderer(
            form, form_predictions)) {
      return;
    }
    // Otherwise, create it and use predictions (which may trigger filling).
    manager = CreateFormManager(driver, form);
    manager->ProcessServerPredictions(predictions_);
    return;
  }

  // If the observed form has changed and is not recognized by the renderer,
  // update the manager and trigger filling.
  if (IsRelevantForPasswordManagerButNotRecognizedByRenderer(
          form, form_predictions) &&
      HasObservedFormChanged(form, *manager)) {
    manager->UpdateFormManagerWithFormChanges(form, predictions_);
    manager->Fill();
    return;
  }

  // Otherwise, just process predictions (which may also trigger filling).
  manager->ProcessServerPredictions(predictions_);
}

PasswordFormManager* PasswordManager::GetSubmittedManager() const {
  if (owned_submitted_form_manager_)
    return owned_submitted_form_manager_.get();

  for (const std::unique_ptr<PasswordFormManager>& manager : form_managers_) {
    if (manager->is_submitted())
      return manager.get();
  }

  return nullptr;
}

absl::optional<PasswordForm> PasswordManager::GetSubmittedCredentials() {
  PasswordFormManager* submitted_manager = GetSubmittedManager();
  if (submitted_manager)
    return submitted_manager->GetPendingCredentials();
  return absl::nullopt;
}

void PasswordManager::ResetSubmittedManager() {
  client_->ResetSubmissionTrackingAfterTouchToFill();

  if (owned_submitted_form_manager_) {
    owned_submitted_form_manager_.reset();
    return;
  }

  auto submitted_manager =
      base::ranges::find_if(form_managers_, &PasswordFormManager::is_submitted);
  if (submitted_manager != form_managers_.end())
    form_managers_.erase(submitted_manager);
}

std::unique_ptr<PasswordFormManagerForUI>
PasswordManager::MoveOwnedSubmittedManager() {
  if (owned_submitted_form_manager_)
    return std::move(owned_submitted_form_manager_);

  for (auto iter = form_managers_.begin(); iter != form_managers_.end();
       ++iter) {
    if ((*iter)->is_submitted()) {
      std::unique_ptr<PasswordFormManager> submitted_manager = std::move(*iter);
      form_managers_.erase(iter);
      return std::move(submitted_manager);
    }
  }

  NOTREACHED();
  return nullptr;
}

void PasswordManager::RecordProvisionalSaveFailure(
    PasswordManagerMetricsRecorder::ProvisionalSaveFailure failure,
    const GURL& form_origin) {
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger = std::make_unique<BrowserSavePasswordProgressLogger>(
        client_->GetLogManager());
  }
  if (client_->GetMetricsRecorder()) {
    client_->GetMetricsRecorder()->RecordProvisionalSaveFailure(
        failure, submitted_form_url_, form_origin, logger.get());
  }
}

// TODO(https://crbug.com/831123): Implement creating missing
// PasswordFormManager when PasswordFormManager is gone.
PasswordFormManager* PasswordManager::GetMatchedManager(
    PasswordManagerDriver* driver,
    FormRendererId form_id) {
  for (auto& form_manager : form_managers_) {
    if (form_manager->DoesManage(form_id, driver))
      return form_manager.get();
  }
  return nullptr;
}

absl::optional<FormPredictions> PasswordManager::FindPredictionsForField(
    FieldRendererId field_id,
    int driver_id) {
  for (const auto& form : predictions_) {
    if (form.second.driver_id != driver_id) {
      continue;
    }
    for (const PasswordFieldPrediction& field : form.second.fields) {
      if (field.renderer_id == field_id) {
        return form.second;
      }
    }
  }
  return absl::nullopt;
}

void PasswordManager::TryToFindPredictionsToPossibleUsernames() {
  for (auto& [unique_identifier, possible_username] : possible_usernames_) {
    if (possible_username.form_predictions) {
      continue;
    }
    possible_username.form_predictions = FindPredictionsForField(
        unique_identifier.renderer_id, unique_identifier.driver_id);
  }
}

void PasswordManager::ShowManualFallbackForSaving(
    PasswordFormManager* form_manager,
    const FormData& form_data) {
  // Where `form_manager` is nullptr, make sure the manual fallback isn't
  // shown. One scenario where this is relevant is when the user inputs some
  // password and then removes it. Upon removing the password, the
  // `form_manager` will become nullptr.
  if (!form_manager) {
    HideManualFallbackForSaving();
    return;
  }

  if (!form_manager->is_submitted())
    return;

  if (!client_->GetProfilePasswordStore()->IsAbleToSavePasswords() ||
      !client_->IsSavingAndFillingEnabled(form_data.url) ||
      ShouldBlockPasswordForSameOriginButDifferentScheme(form_data.url)) {
    return;
  }

  if (!client_->GetStoreResultFilter()->ShouldSave(
          *form_manager->GetSubmittedForm())) {
    return;
  }

  // Show the fallback if a prompt or a confirmation bubble should be available.
  bool is_fallback_for_generated_password =
      ShouldShowManualFallbackForGeneratedPassword(*form_manager);
  if (ShouldPromptUserToSavePassword(*form_manager) ||
      is_fallback_for_generated_password) {
    bool is_update = form_manager->IsPasswordUpdate();
    form_manager->GetMetricsRecorder()->RecordShowManualFallbackForSaving(
        is_fallback_for_generated_password, is_update);
    client_->ShowManualFallbackForSaving(
        form_manager->Clone(), is_fallback_for_generated_password, is_update);
  } else {
    HideManualFallbackForSaving();
  }
}

bool PasswordManager::NewFormsParsed(PasswordManagerDriver* driver,
                                     const std::vector<FormData>& form_data) {
  return base::ranges::any_of(form_data, [driver, this](const FormData& form) {
    return !GetMatchedManager(driver, form.unique_renderer_id);
  });
}

bool PasswordManager::IsFormManagerPendingPasswordUpdate() const {
  for (const auto& form_manager : form_managers_) {
    if (form_manager->IsPasswordUpdate())
      return true;
  }
  return owned_submitted_form_manager_ &&
         owned_submitted_form_manager_->IsPasswordUpdate();
}

#if BUILDFLAG(IS_IOS)
bool PasswordManager::DetectPotentialSubmission(
    PasswordFormManager* form_manager,
    const FieldDataManager& field_data_manager,
    PasswordManagerDriver* driver) {
  // Do not attempt to detect submission if saving is disabled.
  if (!client_->IsSavingAndFillingEnabled(form_manager->GetURL())) {
    RecordProvisionalSaveFailure(
        PasswordManagerMetricsRecorder::SAVING_DISABLED,
        form_manager->GetURL());
    return false;
  }

  // If the manager is not submitted, it still can have autofilled data.
  if (!form_manager->is_submitted()) {
    form_manager->ProvisionallySaveFieldDataManagerInfo(field_data_manager,
                                                        driver);
  }
  // If the manager was set to be submitted, either prior to this function call
  // or on provisional save above, consider submission successful.
  if (IsAutomaticSavePromptAvailable(form_manager)) {
    OnLoginSuccessful();
    return true;
  }
  return false;
}
#endif  // BUILDFLAG(IS_IOS)

}  // namespace password_manager
