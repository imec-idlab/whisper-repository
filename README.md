# Whisper-repository
===================
Collection of software used for implementing Whisper

This repository contains the software used for Whisper proof of concept. It includes:
* The Whisper controller:
	- Algorithms for translating Policies to Primitives"
	- Firmware run in the root"
	- Interface for link the controller with the root node
* Firmware for the Whisper nodes
	- Modified version of OpenWSN that comunicates with the controller through COAP
* Monitoring tools
 	- A sniffer for OpenMote that captures traffic from the network
	- A modified version of Foren6 to process and display information from the captured traffic:
 		- Parent relationship (DAOs)
		- Ranks (DIOs)
 		- 6TiSCH schedules (6P commands)


Issues and bugs
---------------

* Report to esteban.municio@uantwerpen.be
