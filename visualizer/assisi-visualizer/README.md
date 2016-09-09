# Assisi experiment visualization for Ars Electronica 2016

## Description

A quick'n dirty hack to visualize the bees<->fish interconnection@Ars Electronica 2016.
A diagram and short description of the experiment are available [here](https://docs.google.com/drawings/d/1erRWMUPkPlKBw5P2cYpMEhzY0gzC4kwR7TsgFkxvcpY/edit?usp=sharing).

## Assumptions

All connection and layout parameters are currently hardcoded. The following data sources are expected:

- casu-001@bbg-001:1555 (casu data),
- casu-001@bbg-001:10101 (casu messages),
- casu-002@bbg-001:2555 (casu data)
- casu-002@bbg-001:10102 (casu messages)
- cats@cats-workstation:10203 (cats messages)
- FishPosition@cats-workstation:10203 (fish position messages)
- CASUPosition@cats-workstation:10203 (ribot position messages)

CASU IR proximity sensor thresholds are hardcoded!

## Communication protocol

The CASU communication protocol is as documented [here](http://assisipy.readthedocs.io/en/latest/protocol.html)

CASU messages contain a single floating point number (encoded as an utf-8 string), indicating the percentage of
IR proxy sensors that are being triggered.

CATS messages have the following format:

```
<target casu name><CommEth><cats><fish:dir,fishCASU:dir>
```

where `dir` can be `CW` indicating that the majority of fish/ribots are swimming clockwise,
or `CCW` indicating that the majority of the fish/ribots are swimming counterclockwise.

FishPosition and CASUPosition messages are in the following format:

```
<type><id><x><y>
```

where `type` is either `FishPosition` or `CASUPosition`, `id` is an integer `>=0`,
encoded as an utf-8 string, and `x` and `y` are the coordinates, in pixels, in the range `[0,500]`

## TODO

If the code is to be reused for anything else, the following improvements are absulutely necessary:

1. Read CASU details (names, IP adresses, ports, physical layout, IR thresholds) from an .arena file
2. Remove hardcoded stuff from the visual layout (e.g. `casu_top_` etc.)
3. Remove magic numbers
4. Formalize the communication protocol with CATS (proper usage of ZMQ multipart message fields, protobuf encoding)
