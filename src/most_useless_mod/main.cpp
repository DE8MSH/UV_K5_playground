#include "system.hpp"
#include "hardware/hardware.hpp"
#include "registers.hpp"
#include "uv_k5_display.hpp"
#include "spectrum.hpp"
#include "radio.hpp"
#include <string.h>

extern "C" void __libc_init_array();

Radio::CBK4819<System::OrgFunc_01_26> RadioDriver;
CSpectrum<System::OrgFunc_01_26,
          System::OrgData_01_26, RadioDriver>
    Spectrum;

int main()
{
   System::JumpToOrginalFw();
   return 0;
}

extern "C" void MultiIrq_Handler(unsigned int u32IrqSource)
{
   static bool bFirstInit = false;
   if (!bFirstInit)
   {
      System::CopyDataSection();
      __libc_init_array();
      bFirstInit = true;
   }

   RadioDriver.InterruptHandler();

   static unsigned int u32StupidCounter = 1;
   if ((!(u32StupidCounter++ % 16) && u32StupidCounter > 200))
   {
      Spectrum.Handle();
   }

   System::JumpToOrginalVector(u32IrqSource);
}