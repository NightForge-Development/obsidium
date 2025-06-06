// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chromeos/chai_assert.js';

import {reportPromise} from '../../../common/js/test_error_reporting.js';

import {MetadataItem} from './metadata_item.js';
import {MetadataModel} from './metadata_model.js';
import {ThumbnailModel} from './thumbnail_model.js';

const imageEntry = {
  name: 'image.jpg',
  toURL: function() {
    return 'filesystem://A';
  },
};

const nonImageEntry = {
  name: 'note.txt',
  toURL: function() {
    return 'filesystem://B';
  },
};

const contentThumbnailTransform = {
  scaleX: 0,
  scaleY: 0,
  rotate90: 0,
};

const imageTransformation = {
  scaleX: 1,
  scaleY: 1,
  rotate90: 2,
};

// @ts-ignore: error TS7034: Variable 'metadata' implicitly has type 'any' in
// some locations where its type cannot be determined.
let metadata;
// @ts-ignore: error TS6133: 'contentMetadata' is declared but its value is
// never read.
let contentMetadata;
// @ts-ignore: error TS7034: Variable 'thumbnailModel' implicitly has type 'any'
// in some locations where its type cannot be determined.
let thumbnailModel;

export function setUp() {
  metadata = new MetadataItem();
  metadata.modificationTime = new Date(2015, 0, 1);
  metadata.present = true;
  metadata.thumbnailUrl = 'EXTERNAL_THUMBNAIL_URL';
  metadata.customIconUrl = 'CUSTOM_ICON_URL';
  metadata.contentThumbnailUrl = 'CONTENT_THUMBNAIL_URL';
  metadata.contentThumbnailTransform = contentThumbnailTransform;
  metadata.contentImageTransform = imageTransformation;

  thumbnailModel = new ThumbnailModel(/** @type {!MetadataModel} */ ({
    // @ts-ignore: error TS6133: 'entries' is declared but its value is never
    // read.
    get: function(entries, names) {
      const result = new MetadataItem();
      for (let i = 0; i < names.length; i++) {
        const name = names[i];
        // @ts-ignore: error TS2538: Type 'undefined' cannot be used as an index
        // type.
        result[name] = metadata[name];
      }
      return Promise.resolve([result]);
    },
  }));
}

/** @param {()=>void} callback */
export function testThumbnailModelGetBasic(callback) {
  reportPromise(
      // @ts-ignore: error TS7006: Parameter 'results' implicitly has an 'any'
      // type.
      thumbnailModel.get([imageEntry]).then(results => {
        assertEquals(1, results.length);
        assertEquals(
            new Date(2015, 0, 1).toString(),
            results[0].filesystem.modificationTime.toString());
        assertEquals(
            'EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
        assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
        assertTrue(results[0].external.present);
        assertEquals('CONTENT_THUMBNAIL_URL', results[0].thumbnail.url);
        assertEquals(contentThumbnailTransform, results[0].thumbnail.transform);
        assertEquals(imageTransformation, results[0].media.imageTransform);
      }),
      callback);
}

/** @param {()=>void} callback */
export function testThumbnailModelGetNotPresent(callback) {
  // @ts-ignore: error TS7005: Variable 'metadata' implicitly has an 'any' type.
  metadata.present = false;
  reportPromise(
      // @ts-ignore: error TS7006: Parameter 'results' implicitly has an 'any'
      // type.
      thumbnailModel.get([imageEntry]).then(results => {
        assertEquals(1, results.length);
        assertEquals(
            new Date(2015, 0, 1).toString(),
            results[0].filesystem.modificationTime.toString());
        assertEquals(
            'EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
        assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
        assertFalse(results[0].external.present);
        assertEquals(undefined, results[0].thumbnail.url);
        assertEquals(undefined, results[0].thumbnail.transform);
        assertEquals(undefined, results[0].media.imageTransform);
      }),
      callback);
}

/** @param {()=>void} callback */
export function testThumbnailModelGetNonImage(callback) {
  reportPromise(
      // @ts-ignore: error TS7006: Parameter 'results' implicitly has an 'any'
      // type.
      thumbnailModel.get([nonImageEntry]).then(results => {
        assertEquals(1, results.length);
        assertEquals(
            new Date(2015, 0, 1).toString(),
            results[0].filesystem.modificationTime.toString());
        assertEquals(
            'EXTERNAL_THUMBNAIL_URL', results[0].external.thumbnailUrl);
        assertEquals('CUSTOM_ICON_URL', results[0].external.customIconUrl);
        assertTrue(results[0].external.present);
        assertEquals(undefined, results[0].thumbnail.url);
        assertEquals(undefined, results[0].thumbnail.transform);
        assertEquals(undefined, results[0].media.imageTransform);
      }),
      callback);
}
