// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/structured/chrome_structured_metrics_recorder.h"

#include <stdint.h>

#include "base/no_destructor.h"
#include "components/metrics/structured/recorder.h"
#include "components/metrics/structured/structured_metrics_features.h"
#include "components/metrics_services_manager/metrics_services_manager.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ash/login/startup_utils.h"
#include "chrome/browser/browser_process.h"  // nogncheck
#include "chrome/browser/metrics/structured/ash_structured_metrics_recorder.h"  // nogncheck
#include "chrome/browser/metrics/structured/cros_events_processor.h"  // nogncheck
#include "chrome/browser/metrics/structured/event_logging_features.h"  // nogncheck
#include "chrome/browser/metrics/structured/key_data_provider_ash.h"  // nogncheck
#include "chrome/browser/metrics/structured/metadata_processor_ash.h"  // nogncheck
#include "chrome/browser/metrics/structured/oobe_structured_metrics_watcher.h"  // nogncheck
#include "components/metrics/structured/structured_metrics_recorder.h"  // nogncheck
#include "components/metrics/structured/structured_metrics_service.h"  // nogncheck
#elif BUILDFLAG(IS_CHROMEOS_LACROS)
#include "base/task/current_thread.h"
#include "chrome/browser/metrics/structured/lacros_structured_metrics_recorder.h"  // nogncheck
#endif

namespace metrics::structured {
namespace {

// Platforms for which the StructuredMetricsClient will be initialized for.
enum class StructuredMetricsPlatform {
  kUninitialized = 0,
  kAshChrome = 1,
  kLacrosChrome = 2,
};

// Logs initialization of Structured Metrics as a record.
void LogInitializationInStructuredMetrics(StructuredMetricsPlatform platform) {
  events::v2::structured_metrics::Initialization()
      .SetPlatform(static_cast<int64_t>(platform))
      .Record();
}

}  // namespace

ChromeStructuredMetricsRecorder::ChromeStructuredMetricsRecorder() {
// TODO(jongahn): Make a static factory class and pass it into ctor.
#if BUILDFLAG(IS_CHROMEOS_ASH)
  delegate_ = std::make_unique<AshStructuredMetricsRecorder>();
  StructuredMetricsClient::Get()->SetDelegate(this);
#elif BUILDFLAG(IS_CHROMEOS_LACROS)
  delegate_ = std::make_unique<LacrosStructuredMetricsRecorder>();
  StructuredMetricsClient::Get()->SetDelegate(this);
#endif
}

ChromeStructuredMetricsRecorder::~ChromeStructuredMetricsRecorder() = default;

// static
ChromeStructuredMetricsRecorder* ChromeStructuredMetricsRecorder::Get() {
  static base::NoDestructor<ChromeStructuredMetricsRecorder> chrome_recorder;
  return chrome_recorder.get();
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
// static
void ChromeStructuredMetricsRecorder::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  cros_event::CrOSEventsProcessor::RegisterLocalStatePrefs(registry);
}
#endif

void ChromeStructuredMetricsRecorder::Initialize() {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  auto* ash_recorder =
      static_cast<AshStructuredMetricsRecorder*>(delegate_.get());
  ash_recorder->Initialize();

  auto* service = g_browser_process->GetMetricsServicesManager()
                      ->GetStructuredMetricsService();

  // Adds CrOSEvents processor if feature is enabled.
  if (base::FeatureList::IsEnabled(kEventSequenceLogging)) {
    Recorder::GetInstance()->AddEventsProcessor(
        std::make_unique<cros_event::CrOSEventsProcessor>(
            cros_event::kResetCounterPath));

    if (!ash::StartupUtils::IsOobeCompleted()) {
      Recorder::GetInstance()->AddEventsProcessor(
          std::make_unique<OobeStructuredMetricsWatcher>(
              service, GetOobeEventUploadCount()));
    }
  }

  // Initialize the key data provider.
  service->recorder()->InitializeKeyDataProvider(
      std::make_unique<KeyDataProviderAsh>());

  Recorder::GetInstance()->AddEventsProcessor(
      std::make_unique<MetadataProcessorAsh>());

  LogInitializationInStructuredMetrics(StructuredMetricsPlatform::kAshChrome);

#elif BUILDFLAG(IS_CHROMEOS_LACROS)
  auto* lacros_recorder =
      static_cast<LacrosStructuredMetricsRecorder*>(delegate_.get());

  // Ensure that the sequence is the ui thread.
  DCHECK(base::CurrentUIThread::IsSet());
  lacros_recorder->SetSequence(base::SequencedTaskRunner::GetCurrentDefault());
  LogInitializationInStructuredMetrics(
      StructuredMetricsPlatform::kLacrosChrome);
#endif
}

void ChromeStructuredMetricsRecorder::RecordEvent(Event&& event) {
  DCHECK(IsReadyToRecord());
  delegate_->RecordEvent(std::move(event));
}

bool ChromeStructuredMetricsRecorder::IsReadyToRecord() const {
  return delegate_->IsReadyToRecord();
}

}  // namespace metrics::structured
