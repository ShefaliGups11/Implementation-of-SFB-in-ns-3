# Implementation of SFB in ns-3
## Course Code: CS822
## Assignment: #FP3

* This respository contains implementation of SFB queue Disc in NS-3.Stochastic Fair Blue is a classless qdisc to manage congestion based on packet loss and link utilization history while trying to prevent non-responsive flows (i.e. flows that do not react to congestion marking or dropped packets) from impacting performance of responsive flows. Unlike RED, where the marking probability has to be configured, BLUE tries to determine the ideal marking probability automatically. 

