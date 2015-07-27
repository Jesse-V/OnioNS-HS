#OnioNS - the Onion Name System
### Tor-Powered Distributed DNS for Tor Hidden Services

The Onion Name System (OnioNS) is a privacy-enhanced, distributed, and highly usable DNS for Tor hidden services. It allows users to reference a hidden service by a meaningful globally-unique domain name chosen by the hidden service operator. The system is powered by the Tor network and relies on a distributed database. This project aims to address the major usability issue that has been with Tor hidden services since their introduction in 2002. The official project page is onions55e7yam27n.onion, which is example.tor under OnioNS.

### Repository Details

This repository provides functionality for hidden service operator, such as the capability to claim an OnioNS domain name.

### Supported Systems

**Debian 7 and 8, Ubuntu 14.04 - 15.10, Mint 17 - 17.2, Fedora 21 - 23**

Please see the [OnioNS-common README](https://github.com/Jesse-V/OnioNS-common#supported-systems) for more information.

### Installation

There are several methods to install the OnioNS software. The method of choice depends on your system. If you are on Ubuntu or an Ubuntu-based system (Lubuntu, Kubuntu, Mint) please use the PPA method. If you are running Debian Wheezy, please use the .deb method. Otherwise, for all other distributions, please install from source.

* **Install from PPA**

> 1. **sudo add-apt-repository ppa:jvictors/tor-dev**
> 2. **sudo apt-get update**
> 3. **sudo apt-get install tor-onions-hs**

This is the recommended method as it's very easy to stay up-to-date with my releases.

* **Install from .deb file**

I provide builds for Debian Wheezy in the [Releases section](https://github.com/Jesse-V/OnioNS-HS/releases) for several architectures. For other architectures, you may download from [my PPA](https://launchpad.net/~jvictors/+archive/tor-dev/+packages).

* **Install from source**

> 1. Install tor-onions-common by following [these instructions](https://github.com/Jesse-V/OnioNS-common#installation).
> 2. Download and extract the latest release from the [Releases page](https://github.com/Jesse-V/OnioNS-HS/releases).
> 3. *(mkdir build; cd build; cmake ../src; make; sudo make install)*

The rebuild.sh script is available if you are actively developing OnioNS. You will need to install *clang-format-3.6* and *cppcheck* as the script also styles the code and performs a static analysis check.

You can cleanup your build with **rm -rf build**

### Usage

The hidden service edition of the OnioNS software is primarily a command-line utility. Currently it supports the ability to claim a domain name, a number of sub-domains, and send this information over a Tor circuit to the OnioNS network.

> 1. Create a hidden service. See [this guide](https://www.torproject.org/docs/tor-hidden-service) for instructions on how to do this. As this is beta software, I do not recommend using a valuable hidden service in production, so I recommend making a new one for testing purposes.
> 2. **onions-hs --hsKey /var/lib/tor/hidden_service/private_key**
> 3. Follow the on-screen instructions. You may claim a single .tor domain (which will point to your hidden service) and up to 24 subdomains, which must point to a .tor or .onion address of your choosing.

A manpage is available for your convenience.

### Bug Reporting

Please open a ticket on Github. If you do not have a Github account, please contact kernelcorn on #tor-dev on OFTC IRC, or email kernelcorn at riseup dot net. Please follow the same process for filing enhancement requests. I use PGP key 0xC20BEC80. I accept pull requests if you want to contribute.
