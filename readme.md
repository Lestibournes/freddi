# About Freddi
A simple utility for installing and removing flatpaks from a .flatpakref file.

Current state as of 11/09/2022:

![Image](Freddi-2022.09.11.png)

Currently implemented:
* Displaying app metadata
* Installing applications
* Showing installation progress
* Showing that an app is already installed
* Removing apps

Currently not implemented:
* Showing uninstall progress
* Cancelling ongoing operations
* Packaging as Flatpak (might be unfixable)

This is a work in progress, and individual commits might be broken. The app might be unstable.

## Building and Running

Build using the script `build.sh`.

Build and run using the script `run.sh`.

The built executable will be located at `build/src/freddi`. The .desktop file will be at `build/data/com.example.Freddi.desktop`.
Place them in the appropriate locations in your system (for example `~/.local/bin` and `~/.local/share/applications`) to install.

# Usage

Freddi isn't meant to be run on it's own as an app. Instead, use it to open .flatpakref files. You can do it from the right-click menu, or by setting it as the default app for opening .flatpakref from the file properties window. Once it's set as default, you can simply open .flatpakref files from the file manager or browser, and they will open in Freddi, conveniently ready for installation.

## Authors

This project is made by Yitzchak Schwarz.

## License

This project is licensed under GPL-2.0 only.

## Contributing

I would welcome help and advice about developing gnome applications (particularly in C), using AppStream and libflatpak, and packaging as a Flatpak. I also need an icon.