## [HOA](http://www.mshparisnord.fr/hoalibrary/ "High Order Ambisonic Library") for Pure Data

![CaptureHoaPD](http://hoalibrary.mshparisnord.fr/wp-content/uploads/2015/06/CaptureHoaPD.png)

dev/master : [<img src="https://travis-ci.org/CICM/HoaLibrary-PD.svg?branch=dev/master">](https://travis-ci.org/CICM/HoaLibrary-PD "Travis CI") [<img src="https://ci.appveyor.com/api/projects/status/github/CICM/HoaLibrary-PD?branch=dev/master&svg=true">](https://ci.appveyor.com/project/CICM/hoalibrary-pd/history "Appveyor CI")

### Compatibilty :

The lastest release has been tested [Pure Data Vanilla](http://msp.ucsd.edu/software.html "PD-Vanilla") (0.46.6 - 32/64 bits) & [Pure Data Extended](https://puredata.info/ "PD-Extended") (0.43.4) on Linux, Mac Os, Windows.

### Installation :  

Todo

### Documentation :

Helps and tutorials are availables in the <em>Hoa</em> folder of the <em>help browser</em>.

### Compilation :

Requires git and [CMake](https://cmake.org/).

```shell
# clone repository
$ git clone https://github.com/CICM/HoaLibrary-PD.git
$ cd HoaLibrary-PD
$ git submodule update --init --recursive
# generate project
$ mkdir build && cd build
$ cmake ..
# buil project
$ cmake --build . --config Release
```

### Dependencies :

[HoaLibrary](https://github.com/CICM/HoaLibrary-Light)

### Authors :

##### HOA Libary :
- 2012: Pierre Guillot, Eliott Paris & Julien Colafrancesco
- 2012-2015: Pierre Guillot & Eliott Paris
- 2015: Pierre Guillot & Eliott Paris & Thomas Le Meur (Light version)
- 2016: Pierre Guillot & Eliott Paris (Light version)

##### Pure Data implementation :
- 2013-2015: Pierre Guillot & Eliott Paris & Thomas Le Meur

### Licence :

The HOA Library in under the [GNU Public License](http://www.gnu.org/copyleft/gpl.html). If you'd like to avoid the restrictions of the GPL and use Hoa Library for a closed-source product please contact the [CICM](http://cicm.mshparisnord.org/).
