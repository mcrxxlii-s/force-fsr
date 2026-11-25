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

## Usage (v1)

1. Open `fsr_force/fsr_force.ino` in the Arduino IDE, set `R_FIXED`, and upload.
2. Open the Serial Monitor at 115200 baud, with the line ending set to "Newline".
3. Put a known mass on the sensor through a small rigid disc, type the mass in grams (include the disc) and press Enter. Repeat for 3–5 masses.
4. After 2+ points the sketch fits `F = A · G^B` (G = 1/R in µS) and prints the live force `F_N` in newtons. Type `c` to clear.

## Board notes

The board is a clone Nano (ATmega328P with a CH340 USB chip). In the IDE choose **Board: Arduino Nano**, and set **Processor** to **ATmega328P (Old Bootloader)** if the upload fails with `not in sync`.
