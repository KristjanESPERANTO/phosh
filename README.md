# Phosh
[![Code coverage](https://source.puri.sm/Librem5/phosh/badges/master/coverage.svg)](https://source.puri.sm/Librem5/phosh/commits/master)

a trivial wayland shell for prototyping things.

## License

phosh is licensed under the GPLv3+.

## Getting the source

```sh
    git clone https://source.puri.sm/Librem5/phosh
    cd phosh
```

The master branch has the current development version.

## Dependencies
On a Debian based system run

```sh
    sudo apt-get -y install build-essential
    sudo apt-get -y build-dep .
```

For an explicit list of dependencies check the `Build-Depends` entry in the
[debian/control][] file.

If your distro doesn't ship [libhandy](https://source.puri.sm/Librem5/libhandy)
you need to build that from source. More details are in the [gitlab-ci.yml][]
file.

## Building

We use the meson (and thereby Ninja) build system for phosh.  The quickest
way to get going is to do the following:

    meson . _build
    ninja -C _build
    ninja -C _build install

# Testing

To run the tests run

    ninja -C _build test

For details see the *.gitlab-ci.yml* file.

There are some thorough tests not run during CI which can e.g. be run via

    gtester -m thorough  _build/tests/test-idle-manager

## Running
### Running from the source tree
When running from the source tree start the compositor *[phoc][]*
 (*[rootston][]* will do as well). Then start *phosh* using:

    _build/run -U

or in one command:

    ../phoc/_build/run -E '_build/run -U' -C ./data/rootston.ini

When running nested it's recommended to skip the `gnome-session` setup:

    SKIP_GNOME_SESSION=1 ../phoc/_build/run -E '_build/run -U' -C ./data/rootston.ini


This will make sure the needed gsettings schema is found. The '-U' option makes
sure the shell is not locked on startup so you can test with arbitrary
passwords.
This works on hardware as well as nested on other desktop environments. The
result should look something like this:

![phosh](screenshots/phosh.png)

### Running from the Debian packages
If installed via the Debian packages you can also run phosh as a gnome-session.
It ships a file in /usr/share/gnome-session/sessions so you can bring up a
session using

    gnome-session --disable-acceleration-check --session=phosh

If you want to start phosh at system boot there's a systemd unit file in
*/lib/systemd/system/phosh* which is disabled by default:

    systemctl enable phosh
    systemctl start phosh

This runs *phosh* as user *purism* (which needs to exist). If you don't have a
user *purism* and don't want to create one you can make systemd run *phosh* as
any user by using an override file:

    $ cat /etc/systemd/system/phosh.service.d/override.conf
    [Service]
    User=<your_user>
    WorkingDirectory=<your_home_directory>

# Translations
Please use zanata for translations at https://translate.zanata.org/project/view/phosh?dswid=-3784

# Getting in Touch
* Issue tracker: https://source.puri.sm/Librem5/phosh
* Mailing list: https://lists.community.puri.sm/listinfo/librem-5-dev
* Matrix: https://im.puri.sm/#/room/#phosh:talk.puri.sm
* XMPP: phosh@conference.sigxcpu.org

For details see the [developer documentation](https://developer.puri.sm/Contact.html).

[gitlab-ci.yml]: https://source.puri.sm/Librem5/phosh/blob/master/.gitlab-ci.yml
[debian/control]: https://source.puri.sm/Librem5/phosh/blob/master/debian/control
[phoc]: https://source.puri.sm/Librem5/phoc
[rootston]: https://github.com/swaywm/wlroots/tree/master/rootston
