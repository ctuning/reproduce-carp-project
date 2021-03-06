[![compatibility](https://github.com/ctuning/ck-guide-images/blob/master/ck-compatible.svg)](https://github.com/ctuning/ck)
[![automation](https://github.com/ctuning/ck-guide-images/blob/master/ck-artifact-automated-and-reusable.svg)](http://cTuning.org/ae)
[![workflow](https://github.com/ctuning/ck-guide-images/blob/master/ck-workflow.svg)](http://cKnowledge.org)

[![DOI](https://zenodo.org/badge/37073548.svg)](https://zenodo.org/badge/latestdoi/37073548)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

CK repository to reproduce CARP project
=======================================

Shared artifacts (code and data) and workflows
from the [EU FP7 CARP project](http://carp.doc.ic.ac.uk)
in the [Collective Knowledge format (CK)](http://cKnowledge.org)
to let the community reproduce and validate this research.

The vision publication about how Collective Knowledge
can assist scientists to develop sustainable research software 
and help research projects to survive in a Cambrian AI/SW/HW chaos
or when leading developers leave:


```
@inproceedings{ck-date16,
    title = {{Collective Knowledge}: towards {R\&D} sustainability},
    author = {Fursin, Grigori and Lokhmotov, Anton and Plowman, Ed},
    booktitle = {Proceedings of the Conference on Design, Automation and Test in Europe (DATE'16)},
    year = {2016},
    month = {March},
    url = {https://www.researchgate.net/publication/304010295_Collective_Knowledge_Towards_RD_Sustainability}
}
```

* [Open challenges powered by CK](https://github.com/ctuning/ck/wiki/Research-and-development-challenges)

Support
=======
The [non-profit cTuning foundation (France)](http://cTuning.org)
and [dividiti Ltd (UK/US)](http://dividiti.com)
help academic and industrial projects to use
[Collective Knowledge framework (CK)](http://cKnowledge.org) and implement sustainable
and portable research software, share artifacts and workflows as reusable and
customizable components, crowdsource and reproduce experiments,
enable [collaborative AI/SW/HW co-design from IoT to supercomputers](http://cKnowledge.io)
to trade-off speed, accuracy, energy, size and costs,
accelerate knowledge discovery, and facilitate technology transfer.
Contact [them](mailto:grigori.fursin@ctuning.org;anton@dividiti.com) 
for further details.

Prerequisites
=============
* Collective Knowledge Framework (CK): http://github.com/ctuning/ck

Installation
============

```
$ sudo pip install ck
$ ck pull repo:reproduce-carp-project
```

Basic usage
===========

Run experiment on Linux or Windows

```
$ ck compile program:pencil-benchmark-hog --speed
$ ck run program:pencil-benchmark-hog
```

Run experiment on Android with OpenCL:

```
$ ck compile program:realeyes-hog-opencl --speed --target_os=android21-arm64
$ ck run program:realeyes-hog-opencl --speed --target_os=android21-arm64
```
