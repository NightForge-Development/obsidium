// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/device_reauth/ios_device_authenticator.h"

#import "base/notreached.h"

IOSDeviceAuthenticator::IOSDeviceAuthenticator(
    id<ReauthenticationProtocol> reauth_module,
    DeviceAuthenticatorProxy* proxy,
    const device_reauth::DeviceAuthParams& params)
    : DeviceAuthenticatorCommon(proxy,
                                params.GetAuthenticationValidityPeriod(),
                                params.GetAuthResultHistogram()),
      authentication_module_(reauth_module) {}

IOSDeviceAuthenticator::~IOSDeviceAuthenticator() = default;

bool IOSDeviceAuthenticator::CanAuthenticateWithBiometrics() {
  // Currently ReauthenticationModule does not support checking for only the
  // Biometric auth. Considering support that in the future if needed.
  NOTIMPLEMENTED();
  return false;
}

bool IOSDeviceAuthenticator::CanAuthenticateWithBiometricOrScreenLock() {
  return [authentication_module_ canAttemptReauth];
}

void IOSDeviceAuthenticator::AuthenticateWithMessage(
    const std::u16string& message,
    AuthenticateCallback callback) {
  callback_ = std::move(callback);
  bool can_reuse_previous_auth = !NeedsToAuthenticate();
  base::WeakPtr<IOSDeviceAuthenticator> weak_this =
      weak_ptr_factory_.GetWeakPtr();
  auto completion_handler = ^(ReauthenticationResult result) {
    if (weak_this) {
      weak_this->OnAuthenticationCompleted(/*succeeded=*/result !=
                                           ReauthenticationResult::kFailure);
    }
  };

  // TODO(crbug.com/1427216): Populate finalized string here. The mock for this
  // string may need to be updated.
  [authentication_module_
      attemptReauthWithLocalizedReason:@""
                  canReusePreviousAuth:can_reuse_previous_auth
                               handler:completion_handler];
}

void IOSDeviceAuthenticator::Cancel() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  OnAuthenticationCompleted(/*succeeded=*/false);
}

void IOSDeviceAuthenticator::OnAuthenticationCompleted(bool succeeded) {
  CHECK(!callback_.is_null());
  RecordAuthenticationTimeIfSuccessful(succeeded);
  std::move(callback_).Run(succeeded);
}
