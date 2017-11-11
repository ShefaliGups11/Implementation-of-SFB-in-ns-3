# Implementation of SFB in ns-3
## Course Code: CS822
## Assignment: #FP3

## Brief Description:

 This respository contains implementation of SFB queue Disc in NS-3.

## Overview:

Stochastic Fair Blue[2] is a classless qdisc to manage congestion based on packet loss and link utilization history while trying to prevent non-responsive flows (i.e. flows that do not react to congestion marking or dropped packets) from impacting performance of responsive flows. Unlike RED, where the marking probability has to be configured, BLUE tries to determine the ideal marking probability automatically.This repository provides an implementation of SCRED in ns-3.27 [3].

## SFB example

 An example program for Sfb has been provided in

src/traffic-control/examples/sfb-example.cc

  and should be executed as

  ./waf --run "sfb-example"

## References:
[1]http://www.thefengs.com/wuchang/blue/CSE-TR-387-99.pdf

[2]The BLUE Active Queue Management Algorithms (https://mipse.umich.edu/cse/awards/pdfs/blue_active_queue.pdf)

[3]https://www.nsnam.org/ns-3.27/


