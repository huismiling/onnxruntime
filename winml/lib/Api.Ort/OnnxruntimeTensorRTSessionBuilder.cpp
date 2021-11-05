// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "lib/Api.Ort/pch.h"

#ifdef USE_DML

#include "OnnxruntimeTensorRTSessionBuilder.h"
#include "OnnxruntimeEngine.h"
#include "OnnxruntimeErrors.h"
#include "LearningModelDevice.h"

#include "core/providers/tensorrt/tensorrt_provider_factory.h"

using namespace _winml;

HRESULT OnnxruntimeTensorRTSessionBuilder::RuntimeClassInitialize(OnnxruntimeEngineFactory* engine_factory, IExecutionProviderOptions* options) {
  engine_factory_ = engine_factory;
  options_.copy_from(options);
  return S_OK;
}

HRESULT
OnnxruntimeTensorRTSessionBuilder::CreateSessionOptions(
    OrtSessionOptions** options) {
  RETURN_HR_IF_NULL(E_POINTER, options);

  auto ort_api = engine_factory_->UseOrtApi();
  auto winml_adapter_api = engine_factory_->UseWinmlAdapterApi();

  OrtSessionOptions* ort_options;
  RETURN_HR_IF_NOT_OK_MSG(ort_api->CreateSessionOptions(&ort_options),
                          ort_api);

  auto session_options = UniqueOrtSessionOptions(ort_options, ort_api->ReleaseSessionOptions);

  // set the graph optimization level to all (used to be called level 3)
  RETURN_HR_IF_NOT_OK_MSG(ort_api->SetSessionGraphOptimizationLevel(session_options.get(), GraphOptimizationLevel::ORT_ENABLE_ALL),
                          ort_api);

  OrtTensorRTProviderOptions tensorrt_options = {};
  // Request the dml ep
  RETURN_HR_IF_NOT_OK_MSG(ort_api->SessionOptionsAppendExecutionProvider_TensorRT(session_options.get(), &tensorrt_options),
                          ort_api);

#ifndef _WIN64
  auto use_arena = false;
#else
  auto use_arena = true;
#endif
  RETURN_HR_IF_NOT_OK_MSG(winml_adapter_api->OrtSessionOptionsAppendExecutionProvider_CPU(session_options.get(), use_arena),
                          ort_api);

  // call release() so the underlying OrtSessionOptions object isn't freed
  *options = session_options.release();

  return S_OK;
}

HRESULT OnnxruntimeTensorRTSessionBuilder::CreateSession(
    OrtSessionOptions* options,
    OrtSession** session) {
  RETURN_HR_IF_NULL(E_POINTER, session);

  auto ort_api = engine_factory_->UseOrtApi();
  auto winml_adapter_api = engine_factory_->UseWinmlAdapterApi();

  OrtEnv* ort_env;
  RETURN_IF_FAILED(engine_factory_->GetOrtEnvironment(&ort_env));

  OrtSession* ort_session_raw;
  RETURN_HR_IF_NOT_OK_MSG(winml_adapter_api->CreateSessionWithoutModel(ort_env, options, &ort_session_raw),
                          engine_factory_->UseOrtApi());
  auto ort_session = UniqueOrtSession(ort_session_raw, ort_api->ReleaseSession);

  *session = ort_session.release();

  return S_OK;
}

HRESULT OnnxruntimeTensorRTSessionBuilder::Initialize(
    OrtSession* session) {
  RETURN_HR_IF_NULL(E_INVALIDARG, session);
  auto winml_adapter_api = engine_factory_->UseWinmlAdapterApi();

  RETURN_HR_IF_NOT_OK_MSG(winml_adapter_api->SessionInitialize(session),
                          engine_factory_->UseOrtApi());

  return S_OK;
}

#endif USE_DML