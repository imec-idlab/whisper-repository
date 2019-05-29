# Whisper-repository
===================

Collection of software used for using Whisper in a 6TiSCH network

##Contains
---
This repository includes:

1. The Whisper controller
    - Algorithms for translating Policies to Primitives.
    - Local Whisper Controller that runs in Openvisualizer.
    - Coap repository.
    - Sample opologies to test the switch-parent algorithm.
    - Misc tools.
2. Current OpenWSN firmware:
	- Current original OpenWSN firmware for the normal nodes.
3. Firmware for the Whisper nodes:
	- Modified version of OpenWSN that comunicates with the controller through COAP.
4. Firmware for the Whisper root node:
	- Modified version of OpenWSN that comunicates with the controller through serial.
5. Monitoring tools:
 	- A sniffer for OpenMote that captures traffic from the network.
	- A modified version of Foren6 to process and display information from the captured traffic:
 		- Parent relationship (DAOs).
		- Ranks (DIOs).
 		- 6TiSCH schedules (6P commands).
6. ONOS framework with including the Whisper module:
    - Whisper provider.
    - Whisper protocol (southband).
    - Whisper App (GUI+CLI)

##Supports:
---
* Parent switching through DIOs
* Schedule modification through 6P commands (e.g. to allocate cells before the parent switch)
* Monitoring capabilities:
    - Neighbors of the Whisper node
    - Logic and physical topology
* Orchestration from ONOS via edge metrics and intents.

Questions, issues and bugs
---------------

Report to:

* esteban.municio@uantwerpen.be
* niels.balemans@uantwerpen.be

