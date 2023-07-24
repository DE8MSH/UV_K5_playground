#pragma once
#include "radio.hpp"
#include "system.hpp"
#include "uv_k5_display.hpp"
#include <math.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;

template <const System::TOrgFunctions &Fw, const System::TOrgData &FwData,
          Radio::CBK4819<Fw> &RadioDriver>
class CSpectrum {
public:
  static constexpr auto ExitKey = 13;
  static constexpr auto DrawingEndY = 42;
  static constexpr auto BarPos = 5 * 128;


  u8 measurementsCount = 32;
  u8 rssiMin = 255;
  u8 highestPeakX = 0;
  u8 highestPeakT = 0;
  u8 highestPeakRssi = 0;
  u32 highestPeakF = 0;
  u32 FStart;

  CSpectrum()
      : DisplayBuff(FwData.pDisplayBuffer), FontSmallNr(FwData.pSmallDigs),
        Display(DisplayBuff), scanDelay(800), sampleZoom(2), scanStep(25_KHz),
        frequencyChangeStep(100_KHz), rssiTriggerLevel(65) {
    Display.SetFont(&FontSmallNr);
  };

  inline bool ListenPeak() {
    if (highestPeakRssi < rssiTriggerLevel) {
      return false;
    }

    // measure peak for this moment
    highestPeakRssi = GetRssi(highestPeakF); // also sets freq for us
    rssiHistory[highestPeakX >> sampleZoom] = highestPeakRssi;

    if (highestPeakRssi >= rssiTriggerLevel) {
      RadioDriver.ToggleAFDAC(true);
      Listen(1000000);
      return true;
    }

    return false;
  }

  inline void Scan() {
    u8 rssi = 0, rssiMax = 0;
    u8 iPeak = 0;
    u32 fPeak = currentFreq, fMeasure = FStart;

    rssiMin = 255;
    RadioDriver.ToggleAFDAC(false);

    for (u8 i = 0; i < measurementsCount; ++i, fMeasure += scanStep) {
      rssi = rssiHistory[i] = GetRssi(fMeasure);
      if (rssi < rssiMin) {
        rssiMin = rssi;
      }
      if (rssi > rssiMax) {
        rssiMax = rssi;
        fPeak = fMeasure;
        iPeak = i;
      }
    }

    ++highestPeakT;
    if (rssiMax > highestPeakRssi || highestPeakT >= (8 << sampleZoom)) {
      highestPeakT = 0;
      highestPeakRssi = rssiMax;
      highestPeakX = iPeak << sampleZoom;
      highestPeakF = fPeak;
    }
  }

  inline void DrawSpectrum() {
//    for (u8 x = 0; x < 128; ++x) {
//      Display.SetPX(x, 40);
//    }
///////////// MANDEL >>>
          i16 L=100;

                              i16 U;
                              i16 V;
                              i16 X;        
                              i16 Y;
                              i16 N;
                              i16 R;
                              i16 Q;
            for (u8 I = 0; I < 128; ++I) {
                    for (u8 J = 0; J < 56; ++J) {
                              U=I/64-1.5;
                              V=J/28-1;
                              X=U;        
                              Y=V;
                              N=0;
                              R=X*X;
                              Q=Y*Y;
                              if (R+Q>4 || N>=L) {
                                        if (N=>10) {
                                                  Display.SetPX(I, J+8);
                                        }
                              }
                              else {
                              Y=2*X*Y+V;
                              X=R-Q+U;
                              N=N+1;
                              }
                    }          
          }
            

//// < MANDEL
            
  }

  inline void DrawNums() {
    //Display.SetCoursorXY(0, 0);
    //Display.PrintFixedDigitsNumber2(scanDelay, 0);
  }

  inline void DrawRssiTriggerLevel() {
 //   Display.DrawLine(0, 127, Rssi2Y(rssiTriggerLevel));
  }

  inline void DrawTicks() {
    u32 f = modulo(FStart, 1_MHz);
    u32 step = scanStep >> sampleZoom;
    for (u8 i = 0; i < 128; ++i, f += step) {
      u8 barValue = 0b00001000;
      modulo(f, 100_KHz) < step && (barValue |= 0b00010000);
      modulo(f, 500_KHz) < step && (barValue |= 0b00100000);
      modulo(f, 1_MHz) < step && (barValue |= 0b11000000);

      *(FwData.pDisplayBuffer + BarPos + i) |= barValue;
    }

    // center
    *(FwData.pDisplayBuffer + BarPos + 64) |= 0b10101010;
  }

  inline void DrawArrow(u8 x) {

  }

  void HandleUserInput() {
    switch (lastButtonPressed) {
    case 1:
      UpdateScanDelay(200);
      break;
    case 7:
      UpdateScanDelay(-200);
      break;
    case 2:
      UpdateSampleZoom(1);
      break;
    case 8:
      UpdateSampleZoom(-1);
      break;
    case 3:
      UpdateRssiTriggerLevel(5);
      break;
    case 9:
      UpdateRssiTriggerLevel(-5);
      break;
    case 4:
      UpdateScanStep(-1);
      UpdateSampleZoom(1);
      break;
    case 6:
      UpdateScanStep(1);
      UpdateSampleZoom(-1);
      break;
    case 11: // up
      UpdateCurrentFreq(frequencyChangeStep);
      break;
    case 12: // down
      UpdateCurrentFreq(-frequencyChangeStep);
      break;
    case 14:
      UpdateFreqChangeStep(100_KHz);
      break;
    case 15:
      UpdateFreqChangeStep(-100_KHz);
      break;
    default:
      isUserInput = false;
    }
  }

  void Render() {
    DisplayBuff.ClearAll();
//    DrawTicks();
 //   DrawArrow(highestPeakX);
    DrawSpectrum();
//    DrawRssiTriggerLevel();
//    DrawNums();
    Fw.FlushFramebufferToScreen();
  }

  void Update() {
    if (bDisplayCleared) {
      currentFreq = RadioDriver.GetFrequency();
      OnUserInput();
      u16OldAfSettings = Fw.BK4819Read(0x47);
      Fw.BK4819Write(0x47, 0); // mute AF during scan
    }
    bDisplayCleared = false;

    HandleUserInput();

    if (!ListenPeak())
      Scan();
  }

  void UpdateRssiTriggerLevel(i32 diff) {

  }

  void UpdateScanDelay(i32 diff) {

  }

  void UpdateSampleZoom(i32 diff) {

  }

  void UpdateCurrentFreq(i64 diff) {

  }

  void UpdateScanStep(i32 diff) {

  }

  void UpdateFreqChangeStep(i64 diff) {

  }

  inline void OnUserInput() {
    isUserInput = true;
    u32 halfOfScanRange = scanStep << (6 - sampleZoom);
    FStart = currentFreq - halfOfScanRange;

    // reset peak
    highestPeakT = 0;
    highestPeakRssi = 0;
    highestPeakX = 64;
    highestPeakF = currentFreq;

    Fw.DelayUs(90000);
  }

  void Handle() {
    if (RadioDriver.IsLockedByOrgFw()) {
      return;
    }

    if (!working) {
      if (IsFlashLightOn()) {
        working = true;
        TurnOffFlashLight();
      }
      return;
    }

    lastButtonPressed = Fw.PollKeyboard();
    if (lastButtonPressed == ExitKey) {
      working = false;
      RestoreParams();
      return;
    }
    Update();
    Render();
  }


private:
  void RestoreParams() {
    if (!bDisplayCleared) {
      bDisplayCleared = true;
      DisplayBuff.ClearAll();
      Fw.FlushFramebufferToScreen();
      RadioDriver.SetFrequency(currentFreq);
      Fw.BK4819Write(0x47, u16OldAfSettings); // set previous AF settings
    }
  }

  inline void Listen(u32 duration) {
    Fw.BK4819Write(0x47, u16OldAfSettings);
    for (u8 i = 0; i < 16 && lastButtonPressed == 255; ++i) {
      lastButtonPressed = Fw.PollKeyboard();
      Fw.DelayUs(duration >> 4);
    }
    Fw.BK4819Write(0x47, 0);
  }

  u8 GetRssi(u32 f) {
    RadioDriver.SetFrequency(f);
    Fw.DelayUs(scanDelay);
    return Fw.BK4819Read(0x67);
  }

  inline bool IsFlashLightOn() { return GPIOC->DATA & GPIO_PIN_3; }
  inline void TurnOffFlashLight() {
    GPIOC->DATA &= ~GPIO_PIN_3;
    *FwData.p8FlashLightStatus = 3;
  }

  inline u8 Rssi2Y(u8 rssi) {
    return clamp(DrawingEndY - (rssi - rssiMin), 1, DrawingEndY);
  }

  inline i32 clamp(i32 v, i32 min, i32 max) {
    if (v < min)
      return min;
    if (v > max)
      return max;
    return v;
  }

  inline u32 modulo(u32 num, u32 div) {
    while (num >= div)
      num -= div;
    return num;
  }

  TUV_K5Display DisplayBuff;
  const TUV_K5SmallNumbers FontSmallNr;
  CDisplay<const TUV_K5Display> Display;

  u8 lastButtonPressed;
  u32 currentFreq;
  u16 u16OldAfSettings;

  u16 scanDelay;
  u8 sampleZoom;
  u32 scanStep;
  u32 frequencyChangeStep;
  u8 rssiTriggerLevel;

  bool working = false;
  bool isUserInput = false;
  bool bDisplayCleared = true;
};
