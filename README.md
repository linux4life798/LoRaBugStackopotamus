# Description
This project contains the proper LoRaMAC-node port for the LoRaBug and a set of
example programs that use the radio and the LoRaWAN stack. Additionally,
there is an example LoRa basic neighbor MAC implementation called LPLMAC.

# Requirements
Unfortunately, the LoRaMAC-node implementation of LoRaWAN uses C99's in for loop
variable declaration. For this reason, you MUST change the C compiler Language
mode to allow C99 syntax.
Currently, the C99 syntax usage is isolated to the
[LoRaMac.c](LoRaMac-node/src/mac/LoRaMac.c) file.
