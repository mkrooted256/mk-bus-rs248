# mk-bus-rs248
## Low throughput serial bus protocol for AVR MCUs.

__Development in progress.__

Developed as part of home automation system. Inspired by modbus and 1-wire. Speed and low data usage were sacrificed to usability and modularity

Features:
- Master-slave mechanics. Only one device can transmit data simultaneously
- Hot-plugging of slaves __(in progress)__
- Object oriented paradigm, strict separation of low-level code and logic
- Request compilation and response parsing performed by autonomous `Request` objects
- Except some built-in `Request`s, you can also develop your own
- Customizable __(in progress)__

