# FSR force measurement (Arduino Nano)

Measures force with an Ohmite force-sensing resistor (FSR) in a potential divider, read by an Arduino Nano.

## Circuit

```
5V ──[FSR]──┬── A7
            └──[R_fixed ≈ 10 kΩ]── GND
```

The FSR resistance is recovered from the ADC reading (ratiometric, so the exact supply voltage cancels):

```
R_FSR = R_fixed · (1023 − ADC) / ADC
```

Choose `R_fixed ≈ sqrt(R_FSR,min · R_FSR,max)` over the force range you care about. Measure the actual resistor with a multimeter and put the value in `R_FIXED`.

## Sensor model

Power law, which is a straight line on a log-log plot:

```
R = k · F^(−n)      ⇔      ln R = ln k − n · ln F
```

- `k`: FSR resistance at F = 1 N (Ω)
- `n`: log-log slope (roughly 1 for FSRs, but fitted)
- Force from resistance: `F = (k / R)^(1/n)`

The sketch fits `ln R` against `ln F` by least squares. With 3+ points it also prints R², and R² close to 1 confirms the power-law form over the tested range.

## Usage

1. Open `fsr_force/fsr_force.ino` in the Arduino IDE, set `R_FIXED`, and upload.
2. Open the Serial Monitor at 115200 baud, with the line ending set to "Newline".
3. Put a known mass on the sensor through a small rigid disc, type the mass in grams (include the disc) and press Enter. Repeat for 3–5 masses spread over your range. Type `c` to clear.
4. The sketch prints `k`, `n` and R², then the live force `F_N` in newtons.

### Keeping the calibration

Opening the serial port resets the Nano, and the calibration lives in RAM, so a reset wipes it. After calibrating, copy the printed `k` and `n` into `K_PRESET` / `N_PRESET` and re-upload.

### Live graph

Set `PLOT_MODE = true`, re-upload, and open the Serial Plotter. It prints one force value per line (or `V` before calibration).

## Board notes

The board is a clone Nano (ATmega328P with a CH340 USB chip). In the IDE choose **Board: Arduino Nano**, and set **Processor** to **ATmega328P (Old Bootloader)** if the upload fails with `not in sync`.

## Versions

- **v1**: self-calibrating fit `F = A · G^B` on conductance.
- **v2**: fit `R = k · F^(−n)` directly (prints k, n, R²), adds plot mode and a saved calibration.
