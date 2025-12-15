#include <Mozzi.h>
#include <Oscil.h>
#include <tables/cos8192_int8.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <FixMath.h>

// ---------- potentiometers ----------
#define POT_DETUNE   A0
#define POT_CHANCE   A1
#define POT_GLOBAL   A2

uint16_t potDetune = 0;
uint16_t potChance = 0;
uint16_t potGlobal = 0;

// ---------- oscillators ----------
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos1(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos2(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos3(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos4(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos5(COS8192_DATA);

Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos1b(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos2b(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos3b(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos4b(COS8192_DATA);
Oscil<COS8192_NUM_CELLS, MOZZI_AUDIO_RATE> aCos5b(COS8192_DATA);

// ---------- base frequencies ----------
UFix<12,15> f1,f2,f3,f4,f5;

// ---------- correlated drift ----------
SFix<4,12> drift = 0;
UFix<3,16> globalOffset = 0;

// ---------- detune variation ----------
UFix<3,16> variation()
{
  uint32_t r = xorshift96() & 524287UL;
  r = (r * potDetune) >> 10;
  return UFix<3,16>::fromRaw(r);
}

void setup(){
  startMozzi();

  f1 = mtof(UFix<7,0>(48));
  f2 = mtof(UFix<7,0>(74));
  f3 = mtof(UFix<7,0>(64));
  f4 = mtof(UFix<7,0>(77));
  f5 = mtof(UFix<7,0>(67));

  aCos1.setFreq(f1);
  aCos2.setFreq(f2);
  aCos3.setFreq(f3);
  aCos4.setFreq(f4);
  aCos5.setFreq(f5);

  aCos1b.setFreq(f1);
  aCos2b.setFreq(f2);
  aCos3b.setFreq(f3);
  aCos4b.setFreq(f4);
  aCos5b.setFreq(f5);
}

void loop(){
  audioHook();
}

void updateControl(){
  potDetune = mozziAnalogRead(POT_DETUNE);
  potChance = mozziAnalogRead(POT_CHANCE);
  potGlobal = mozziAnalogRead(POT_GLOBAL);

  // ---------- correlated micro drift ----------
  drift = drift + SFix<4,12>::fromRaw(
    (int16_t)(xorshift96() & 31) - 16
  );

  // ---------- rare macro pitch shift ----------
  if ((xorshift96() & 16383) < potGlobal) {
    globalOffset = UFix<3,16>::fromRaw(xorshift96() & 131071UL);
  }

  // ---------- chance-gated oscillator updates ----------
  if ((xorshift96() & 1023) < potChance) {
    switch (lowByte(xorshift96()) % 5){
      case 0: aCos1b.setFreq(f1 + globalOffset + variation() + drift); break;
      case 1: aCos2b.setFreq(f2 + globalOffset + variation() + drift); break;
      case 2: aCos3b.setFreq(f3 + globalOffset + variation() + drift); break;
      case 3: aCos4b.setFreq(f4 + globalOffset + variation() + drift); break;
      case 4: aCos5b.setFreq(f5 + globalOffset + variation() + drift); break;
    }
  }
}

AudioOutput updateAudio(){
  auto asig =
    toSFraction(aCos1.next()) + toSFraction(aCos1b.next()) +
    toSFraction(aCos2.next()) + toSFraction(aCos2b.next()) +
    toSFraction(aCos3.next()) + toSFraction(aCos3b.next()) +
    toSFraction(aCos4.next()) + toSFraction(aCos4b.next()) +
    toSFraction(aCos5.next()) + toSFraction(aCos5b.next());

  return MonoOutput::fromSFix(asig);
}
