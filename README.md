# Flipper Zero BT Serial Example App

This is a very simple Serial-over-Bluetooth app for Flipper Zero. You can use it (but better not to :D) as a reference app for building your own solutions. 

Special thanks to [Willy-JL](https://github.com/Willy-JL) for his help in getting the BLE stack to work, and the reference code from [Flipper PC Monitor](https://github.com/TheSainEyereg/flipper-pc-monitor) by [TheSainEyereg](https://github.com/TheSainEyereg).

## What does it do?

This application hijacks the default Serial-over-Bluetooth connection of a Flipper Zero device (that is usually used for communication with mobile app) and set a custom callback on data received over the connection. Therefore, by modifying this app you can implement your own custom logic on using Serial connection over Bluetooth. As an example - see this repository: https://github.com/maybe-hello-world/flipper-bp

## How to use?

0. Optional: enable debug log on the Flipper device: System - Log level - Debug
1. Compile and upload the app on the flipper.
2. Enable bluetooth and pair your device with the flipper.
3. Run the application on the flipper (optionally: connect flipper via usb and use `fbt cli -> log` to observe the logs)
4. Use bluetooth terminal to send some data. For example with `bluetoothctl` on Linux:
    ```shell
    $ bluetoothctl
    scan on
    # Wait till you see your device, with the name "Serial YourFlipperName"
    scan off

    pair YourFlipperMAC
    # You would need to input the pin code shown on the flipper screen
    trust YourFlipperMAC
    connect YourFlipperMAC

    menu gatt
    list-attributes
    # You should see one that is like
    # Characteristic (Handle 0x0000)
    #         /org/bluez/hci0/dev_80_E1_26_73_69_46/service000c/char000d
    #         19ed82ae-ed21-4c9d-4145-228e62fe0000
    #         Vendor specific
    select-attribute /org/bluez/hci0/dev_80_E1_26_73_69_46/service000c/char000d
    write "0x01 0x02 0x03 0x04"
    ```

## How to modify it for my own app?

To see how this work in real-world app, take a look at the [Flipper PC Monitor](https://github.com/TheSainEyereg/flipper-pc-monitor) by [TheSainEyereg](https://github.com/TheSainEyereg). This app is just a debugging point for BLE functions.

## How to build?

See [this](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/AppsOnSDCard.md) official documentation on how to build external applications for Flipper Zero.
