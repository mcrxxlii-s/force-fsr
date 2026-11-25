// FSR force measurement: potential divider + Arduino Nano
//
// Wiring:   5V ──[FSR]──┬── A7
//                       └──[R_FIXED]── GND
//
// Sensor model (power law):   R = k * F^(-n)
//   k = FSR resistance at F = 1 N  (ohms)
//   n = slope of the line on a log-log plot (FSRs: roughly 1, but fit it)
//   Fitted as a straight line:  ln R = ln k - n ln F
//   Then force from resistance: F = (k / R)^(1/n)
//
// Serial Monitor: 115200 baud, line ending = "Newline"
//   1. Put a known mass on the sensor (through a small rigid disc).
//   2. Type its mass in grams (include the disc), e.g. 200, press Enter.
//   3. Repeat for 3-5 masses spread over your range.
//      After 2+ points it prints k, n and R^2, then live force in N.
//      R^2 close to 1 (with 3+ points) = the power-law form is confirmed.
//   Type  c  to clear and start again.
//
// Displaying force:
//   - Serial Monitor: the F_N column (newtons), once calibrated.
//   - Serial Plotter (live graph): set PLOT_MODE = true and re-upload.
//   - Opening the serial port RESETS the Nano and wipes the calibration (it lives in RAM).
//     So after calibrating, copy the printed k and n into K_PRESET / N_PRESET and re-upload.

const int   FSR_PIN = A7;        // change to A0 etc. if you wire it elsewhere
const float R_FIXED = 10000.0;   // ohms: MEASURE your resistor with a multimeter, put it here
const float ADC_MAX = 1023.0;    // Nano: 10-bit ADC
const float VCC     = 5.0;       // only used to print V_out
const float G_ACC   = 9.81;      // m/s^2

const bool  PLOT_MODE = false;   // true = print only the force, for Serial Plotter
const float K_PRESET  = 0.0;     // after calibrating: paste k (ohm) here
const float N_PRESET  = 0.0;     // after calibrating: paste n here

const int MAX_PTS = 10;
float xLnF[MAX_PTS], yLnR[MAX_PTS];
int   nPts = 0;
float kFit = 0, nFit = 0, r2 = 0;
bool  calibrated = false;

float readAdc() {
  long s = 0;
  for (int i = 0; i < 16; i++) s += analogRead(FSR_PIN);
  return s / 16.0;
}

// Least squares:  y = c + s x,  with x = ln F (known), y = ln R (measured)
//   s = -n,  c = ln k
void fit() {
  if (nPts < 2) { calibrated = false; return; }
  float sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0;
  for (int i = 0; i < nPts; i++) {
    sx  += xLnF[i];          sy  += yLnR[i];
    sxx += xLnF[i] * xLnF[i]; sxy += xLnF[i] * yLnR[i];
    syy += yLnR[i] * yLnR[i];
  }
  float dx = nPts * sxx - sx * sx;
  float dy = nPts * syy - sy * sy;
  if (fabs(dx) < 1e-9) { calibrated = false; return; }   // all masses equal
  float slope = (nPts * sxy - sx * sy) / dx;
  float c     = (sy - slope * sx) / nPts;
  nFit = -slope;
  kFit = exp(c);
  float num = nPts * sxy - sx * sy;
  r2 = (fabs(dy) < 1e-12) ? 1.0 : (num * num) / (dx * dy);
  calibrated = (nFit > 0);
}

void handleSerial(float R) {
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
  if (R <= 0) { Serial.println("# no contact / saturated - check the mass is on the sensor"); return; }
  if (nPts >= MAX_PTS) { Serial.println("# max points reached, type c to restart"); return; }

  float F = grams / 1000.0 * G_ACC;   // newtons
  xLnF[nPts] = log(F);
  yLnR[nPts] = log(R);
  nPts++;
  fit();

  Serial.print("# point "); Serial.print(nPts);
  Serial.print(": m="); Serial.print(grams); Serial.print(" g, F=");
  Serial.print(F, 3); Serial.print(" N, R="); Serial.print(R, 0); Serial.println(" ohm");
  if (calibrated) {
    Serial.print("# fit: R = k * F^-n   k = "); Serial.print(kFit, 1);
    Serial.print(" ohm (R at 1 N),  n = "); Serial.print(nFit, 4);
    if (nPts >= 3) { Serial.print(",  R^2 = "); Serial.print(r2, 4); }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  if (K_PRESET > 0 && N_PRESET > 0) {          // use saved calibration
    kFit = K_PRESET; nFit = N_PRESET; calibrated = true;
  }
  if (!PLOT_MODE) {
    Serial.println("# FSR ready. Type a mass in grams to add a calibration point, c to clear.");
  }
}

void loop() {
  float adc = readAdc();
  float V   = adc * VCC / ADC_MAX;
  float R   = -1;    // ohms

  if (adc >= 3 && adc < ADC_MAX - 0.5) {
    R = R_FIXED * (ADC_MAX - adc) / adc;   // R_fsr = R_fixed * (Vcc - Vout) / Vout
  }

  handleSerial(R);

  if (PLOT_MODE) {                                // one number per line -> clean graph
    if (!calibrated)  { Serial.print("V:");  Serial.println(V, 3); }
    else if (adc < 3) { Serial.println("F_N:0"); }
    else if (R > 0)   { Serial.print("F_N:"); Serial.println(pow(kFit / R, 1.0 / nFit), 3); }
    delay(50);
    return;
  }

  Serial.print("V:"); Serial.print(V, 3);
  if (adc < 3) {
    Serial.println("\tR:open\tF_N:0");                 // nothing pressing (or R_FIXED too small)
  } else if (adc >= ADC_MAX - 0.5) {
    Serial.println("\tR:SAT");                         // R_fsr << R_FIXED: use a SMALLER R_FIXED
  } else {
    Serial.print("\tR_ohm:"); Serial.print(R, 0);
    if (calibrated) { Serial.print("\tF_N:"); Serial.println(pow(kFit / R, 1.0 / nFit), 3); }
    else            { Serial.println("\tF_N:uncal"); }
  }
  delay(200);
}
