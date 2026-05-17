#!/bin/bash

/usr/lib/qt6/bin/qdbus --system --literal org.freedesktop.login1 /org/freedesktop/login1/user/self org.freedesktop.login1.User.Terminate
