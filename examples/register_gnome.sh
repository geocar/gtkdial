#!/bin/sh
gt="gconftool-2"
p="`which dial_helper`"
if [ -f "$p" -a -x "$p" ]; then
	$gt -s /desktop/gnome/url-handlers/tel/command "$p %s" --type String
	$gt -s /desktop/gnome/url-handlers/tel/enabled --type Boolean true
fi
