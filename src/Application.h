#pragma once

#include "globals.h"
#include "config/PinConfig.h"
#include "config/AppConfig.h"

// 만약 PwmFan.h가 따로 있다면 추가해 주세요.
// #include "devices/PwmFan.h" 

namespace incubator
{

class Application
{
public:
    Application();
    
    // 초기화 및 메인 루프 함수
    void init();
    void tick();

private:
    // 하드웨어 및 통신 디바이스
    devices::I2cBus         m_i2c;
    devices::Aht20Driver    m_aht20;
    devices::GpioOutput     m_heater;
    devices::GpioOutput     m_humidifier;
    devices::GpioOutput     m_turner;
    devices::GpioOutput     m_buzzer;
    // 이전 대화에서 PwmFan으로 수정하셨다면 타입을 PwmFan으로 맞춰주세요.
    devices::PwmFan         m_fan; 
    devices::St7789Display  m_display;
    devices::Ec11Encoder    m_encoder;

    // 도메인 데이터 모델
    domain::AppSettings         m_settings;
    domain::RuntimeState        m_state;
    domain::IncubationBatch     m_batch;
    domain::IncubationPlanTable m_plan;

    // 스토리지 계층
    storage::NvsStorage         m_nvs;
    storage::PlanStorage        m_planStorage;

    // 비즈니스 로직 모듈
    modules::SensorManager       m_sensorMgr;
    modules::IncubationScheduler m_scheduler;
    modules::ClimateModule       m_climate;
    modules::TurningModule       m_turning;

    app::AppController           m_appCtrl;
    infra::ProvisioningManager   m_provisioning;

    // UI 계층
    ui::UiModel                  m_uiModel;
    ui::UiController             m_uiCtrl;
    ui::MainUiRenderer           m_renderer;

#ifdef INCUBATOR_ENABLE_CLOUD
    cloud::WifiManager           m_wifiMgr;
    cloud::AwsIotClient          m_awsClient;
#endif
};

} // namespace incubator