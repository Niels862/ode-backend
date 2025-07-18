Introduction
============

This documentation was written on the basis of the reverse engineering process of the bachelor thesis of Niels van Schagen. 

As a prerequisite, the reader is assumed to be familiar with the structure of the configuration array, which is detailed in the User Manual.

Addressing
==========
Bank and byte addresses will be indicated with hexadecimal notation:
6:12 refers to the byte in bank 6 at address 0x12.

For CAB banks, the notation a:XX and b:XX refers to bytes in Bank A and B, respectively.

Channels
========

The following channels are present. Each channel has both a primary and secondary "side", which can be used independently. 

- from IO1/2 direct to CAB1 (input)
- from IO1/2 direct to CAB2 (input)
- from IO3/4 direct to CAB3 (input)
- from IO3/4 direct to CAB4 (input)

- from CAB1 direct to IO1/2 (output)
- from CAB2 direct to IO1/2 (output)
- from CAB3 direct to IO3/4 (output)
- from CAB4 direct to IO3/4 (output)

- from IO indirect to CAB1/3 (bidirectional) 
- from IO indirect to CAB2/4 (bidirectional) 

from each CAB to each other CAB, so 4 x 3 = 12 lines

Local (per CAB):
- local input channels
- local output channels
- opamp output channels

Clocks
======

Each CAB can select two system clocks for synchronizing its compponents. These are set in byte b:00. The byte equals (ID_B << 4) | (ID_A) where ID_x is a 4-bit word indicating a clock. The following mapping is used:

	case 0: return 0x0; // disabled clock
	case 1: return 0xC;
	case 2: return 0xD;
	case 3: return 0xE;
	case 4: return 0xF;
	case 5: return 0xA;
	case 6: return 0xB;

The byte 0:06 controls which clocks are used. It has the following bit structure:

MSB
(6)(5)(4)(3)(2)(1)'0''1'
Where (x) is '1' if the clock is used and '0' otherwise. 

The clock frequencies and delays can be configured. This is implemented in the compiler, but the data is held static. Please refer to `AnalogChip::compile_clocks` and `Clock::compile` for more information.

Switches & Capacitors
=====================

The capacitance of each capacitor is set through one byte. The capacitance is usually set according to an equation (found in the CAM documentation of AD2).

For example, C1 / C2 = 0.5 would yield C1 = 127 and C2 = 254. In the case that multiple values result in the same error, the highest capacitance is chosen.

Switches are configured through four bytes, where the first two bytes control the input switch and the other two bytes control the output switch. The input switch selects the input to its capacitor, and the output switch selects its output. Each capacitor therefore has two programmable switches.

Multiple states can be selected. For both switches, the switch can be opned (disconnected) using "00 00". 

The input switch can be connected to an input using "00 X0", where X can be found in the following table. 

```
0 	0000	-> 	None		
1	0001	-> 	Ground

2   0010	-> 	Local Op-amp 2	
3	0011	-> 	Local Op-amp 1

4	0100 	-> 	Unused
5	0101	-> 	Unused 	

6	0110	-> 	Local input 2	
7	0111	-> 	Local input 1

8	1000	-> 	io1/io2:2 for CAB 1, 2
				CAB2:Op-amp2 for CAB 3, 4
9	1001	-> 	io1/io2:1 for CAB 1, 2
				CAB2:Op-amp1 for CAB 3, 4

A	1010	-> 	CAB1:Op-amp2 for CAB 2, 3, 4
				CAB3:Op-amp2 for CAB 1
B	1011	->	CAB1:Op-amp1 for CAB 2, 3, 4
				CAB3:Op-amp1 for CAB 1

C	1100	->	CAB3:Op-amp2 for CAB 2, 4
				CAB4:Op-amp2 for CAB 10
				IO3/IO4:1 for CAB 3
D	1101	->	CAB3:Op-amp1 for CAB 2, 4
				CAB4:Op-amp1 for CAB 1
				IO3/IO4:2 for CAB 3

E	1110 	->	CAB2:Op-amp2 for CAB 1
				CAB4:Op-amp2 for CAB 2, 3
				IO3/IO4:2 for CAB 4
F	1111	->	CAB2:Op-amp1 for CAB 1
				CAB4:Op-amp1 for CAB 2, 3
				IO3/IO4:1 for CAB 4
```

The switch can also be synchronized on a clock. One of two clocks selected for a CAB can be chosen. "01 YX" uses Clock A, while "02 YX" uses Clock B. Here, the switch is connected to X in phase 1 and to Y in phase 2. 

As an example, the switch `01 13` is connected to Op-amp 1 in phase 1 and to ground on phase 2. 

More configuratIOns are available through comparator control:

- Swap if Control > 0: 3D YX 
- Swap if Control < 0: 2D YX

These follow the same setup as the synchronizing clock, but swap the connections when the condition is met. Please note that the comparator configuration is more experimental, as only few modules utilize it and it was not the focus of the reverse engineering process. The given values are valid for Clock A, but the setup for Clock B is unknown. However, it is likely that this will change 3D / 2D to 3E / 2E, following the clock selection pattern. This configuration is used by the GainSwitch CAM.

Output switches are more limited in configuration. The following options are available:
- Static open: 00 00
- Static closed: 00 N0
- Clock synchronized: 
    - 0S 8N for signal path on phase 1
    - 0S N8 for signal path on phase 2

Here, S is the same clock selector value as previously and N is the op-amp selector, equal to 1 for op-amp 1 and 2 for op-amp 2.  

Capacitor Memory Addresses
--------------------------

| Capacitor | Capacitance   | Switch    |
|-----------|---------------|-----------|
| C1        | a:07          | b:1C      |
| C2        | a:06          | a:1C      |
| C3        | a:05          | b:16      |
| C4        | a:04          | a:18      |
| C5        | a:03          | b:10      |
| C6        | a:02          | a:14      |
| C7        | a:01          | b:0C      |
| C8        | a:00          | a:10      |

Op-Amp
======

