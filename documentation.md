Introduction
============

This documentation was written on the basis of the reverse engineering process of the bachelor thesis of Niels van Schagen. 

As a prerequisite, the reader is assumed to be familiar with the structure of the configuration array, which is detailed in the User Manual.

Addressing
==========
Bank and byte addresses will be indicated with hexadecimal notation:
6:12 refers to the byte in bank 6 at address 0x12.

For CAB banks, the notation a:XX and b:XX refers to bytes in Bank A and B, respectively.

Switches & Capacitors
=====================

The capacitance of each capacitor is set through one byte. 

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

