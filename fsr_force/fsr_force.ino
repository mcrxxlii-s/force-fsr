// FSR force measurement: potential divider + Arduino Nano
//
// Wiring:   5V ──[FSR]──┬── A7
//                       └──[R_FIXED]── GND
//
// Serial Monitor: 115200 baud, line ending = "Newline"
//   1. Put a known mass on the sensor (through a small rigid disc).
//   2. Type its mass in grams (include the disc), e.g. 200, press Enter.
//   3. Repeat for 3-5 different masses. After 2+ points the sketch fits
//      F = A * G^B  (G = conductance in uS) and starts printing force in N.
//   Type  c  to clear the calibration and start again.

const int   FSR_PIN = A7;        // change to A0 etc. if you wire it elsewhere
const float R_FIXED = 10000.0;   // ohms: MEASURE your resistor with a multimeter, put it here
const float ADC_MAX = 1023.0;    // Nano: 10-bit ADC
const float VCC     = 5.0;       // only used to print V_out
const float G_ACC   = 9.81;      // m/s^2

const int MAX_PTS = 10;
float lnG[MAX_PTS], lnF[MAX_PTS];
int   nPts = 0;
float A = 0, B = 0;
bool  calibrated = false;

float readAdc() {
  long s = 0;
  for (int i = 0; i < 16; i++) s += analogRead(FSR_PIN);
  return s / 16.0;
}

// Least squares on  ln F = ln A + B ln G
void fit() {
  if (nPts < 2) { calibrated = false; return; }
  float sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (int i = 0; i < nPts; i++) {
    sx += lnG[i]; sy += lnF[i];
    sxx += lnG[i] * lnG[i]; sxy += lnG[i] * lnF[i];
  }
  float den = nPts * sxx - sx * sx;
  if (fabs(den) < 1e-9) { calibrated = false; return; }
  B = (nPts * sxy - sx * sy) / den;
  A = exp((sy - B * sx) / nPts);
  calibrated = true;
}

void handleSerial(float G_uS) {
  if (!Serial.available()) return;
  String s = Serial.readStringUntil('\n');
  s.trim();
  if (s == "c") {
    nPts = 0; calibrated = false;
    Serial.println("# calibration cleared");
    return;
  }
  float grams = s.toFloat();
  if (grams <= 0) return;
  if (G_uS <= 0) { Serial.println("# no contact / saturated - check the mass is on the sensor"); return; }
  if (nPts >= MAX_PTS) { Serial.println("# max points reached, type c to restart"); return; }

  float F = grams / 1000.0 * G_ACC;   // newtons
  lnG[nPts] = log(G_uS);
  lnF[nPts] = log(F);
  nPts++;
  fit();

  Serial.print("# point "); Serial.print(nPts);
  Serial.print(": m="); Serial.print(grams); Serial.print(" g, F=");
  Serial.print(F, 3); Serial.print(" N, G="); Serial.print(G_uS, 2); Serial.println(" uS");
  if (calibrated) {
    Serial.print("# fit: F = "); Serial.print(A, 6);
    Serial.print(" * G^"); Serial.println(B, 4);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  Serial.println("# FSR ready. Type a mass in grams to add a calibration point, c to clear.");
}

void loop() {
  float adc = readAdc();
  float V   = adc * VCC / ADC_MAX;
  float R   = -1;    // ohms
  float G   = 0;     // microsiemens

  if (adc >= 3 && adc < ADC_MAX - 0.5) {
    R = R_FIXED * (ADC_MAX - adc) / adc;   // R_fsr = R_fixed * (Vcc - Vout) / Vout
    G = 1.0e6 / R;
  }

  handleSerial(G);

  Serial.print("V:"); Serial.print(V, 3);
  if (adc < 3) {
    Serial.println("\tR:open\tF_N:0");                 // nothing pressing
  } else if (adc >= ADC_MAX - 0.5) {
    Serial.println("\tR:SAT");                         // R_fsr << R_FIXED: use a SMALLER R_FIXED
  } else {
    Serial.print("\tR_ohm:"); Serial.print(R, 0);
    Serial.print("\tG_uS:");  Serial.print(G, 2);
    if (calibrated) { Serial.print("\tF_N:"); Serial.println(A * pow(G, B), 3); }
    else            { Serial.println("\tF_N:uncal"); }
  }
  delay(200);
}
