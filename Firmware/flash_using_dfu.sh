#!/bin/bash

set -e
dfu-util --alt 0 --download ${1:-[none]}/${2:-[none]}.dfu