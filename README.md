# aGrip

aGrip is one of the 4 projects of Saint-Pierre HumanLab Fabrikarium 2024. The initial goal was to help Rime, a 6 years old youg lady 
with her walking frame. Rime use to unintentionally drop her left arm from it while walking and can loose balance.

The technical solution the team retained is to use a M5 Stick with a Force Sensitive Resistor sensor (FSR) to send a buzzer signal 
when the left hand pressure is not strong enough.

To avoid inopportune buzzing, we decided to add an extra FSR to the right hand as a signal to say "don't buzz if none of my hands are hanging"

## Requirements

- 2 M5 Stick Plus 2
- 2 FSR sensors
- 1 buzzer
- Wiring gear

## Directories organization

* `code/`: Arduino code for both left and right hands
* `cao/`: 3D designs


## Usefull links

* M5Stick plus 2 documentation: <https://docs.m5stack.com/en/core/M5StickC%20PLUS2>
 