# lab-scpi

> Easily configurable lab assistant that logs data using SCPI protocol

## Workflow

1. Connect all of your SCPI-compatible test equipments (DMMs, AFGs, oscilloscopes, DC power supplies, spectrum analyzers, etc.) to a central computer. Supported connection methods include:

    * SCPI over TTY over USB (`/dev/ttyUSB0`)
    * SCPI over TCP over LAN (IP address + port)
    * (TODO) SCPI over USBTMC (`/dev/usbtmc0`)

1. Write a YAML configuration describing your setup:

    ```yaml
    channels:
      dmm: !tcp
        host: 10.11.13.200
        port: 5025
      ps: !tty
        dev: /dev/ttyUSB0
        baud: 9600
        stop: 2
    ```

1. In the same YAML file, write your testing plan:

    ```yaml
    steps:
      - !send {channel: ps, cmd: 'OUTP ON'}
      - !delay {seconds: 1}
      - !send {channel: dmm, cmd: 'MEAS:VOLT:DC?'}
      - !recv {channel: dmm, unit: V}
      - !send {channel: ps, cmd: 'OUTP OFF'}
    ```

1. Download and compile `lab-scpi`:

    ```bash
    git get b1f6c1c4/lab-scpi
    cd lab-scpi
    cmake -G Ninja -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
    cmake --build cmake-build-release
    ```

1. Run the executable:

    ```bash
    ./cmake-build-release/lab <path-to-config>.yaml <path-to-data>.json
    ```

