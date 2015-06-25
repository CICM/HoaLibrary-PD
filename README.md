## [HOA](http://www.mshparisnord.fr/hoalibrary/ "Hoa Library") for Pure Data

![CaptureHoaPD](http://www.mshparisnord.fr/hoalibrary/wp-content/uploads/2015/06/CaptureHoaPD.png)

### Compatibilty :

The lastest release has been tested [Pure Data Vanilla](http://msp.ucsd.edu/software.html "PD-Vanilla") (0.46.6 - 32/64 bits) & [Pure Data Extended](https://puredata.info/ "PD-Extended") (0.43.4) on Linux, Mac Os, Windows .

### Installation :  

With Pure Data Vanilla, copy the <em>Hoa</em> folder in your package folder and add <em>hoa</em> in the PD's statup window if you use the default package folder\*, otherwise add <em>Hoa/hoa</em>.  

With Pure Data Extended, copy the <em>Hoa</em> folder in your package folder and add <em>-lib externals/Hoa</em> in the statup falgs if you use the default package folder\*, otherwise add <em>-lib Hoa/Hoa</em>.  

\* The default package folder are generally <em>/usr/local/lib/pd-externals</em> on Linux, <em>/Library/Pd</em>  on Mac Os and <em>C:\Program Files\Common Files\Pd</em>  on Windows.  

__Important__ : The Hoa library needs the [Cream library](https://github.com/CICM/CreamLibrary "Cream") to work properly.

### Documentation :

Helps and tutorials are availables in the <em>Hoa</em> folder of the <em>help browser</em>.
 
### Compilation : 

	./autogen.sh (if needed)
	./configure or ./configure --with-pdextended
	make
	make install (optional)

XCode, CodeBlock and Visual Studio projects are also available.

### Dependencies : 

[Hoa Library](https://github.com/CICM/HoaLibrary-Light "Hoa Library") & the [Cicm Wrapper](https://github.com/CICM/CicmWrapper "Cicm Wrapper").

### Authors :

Pierre Guillot  
Eliott Paris  
Thomas Le Meur  
Julien Colafrancesco

### Licence : 

The HOA Library in under the <a title="GNU" href="http://www.gnu.org/copyleft/gpl.html" target="_blank">GNU Public License</a>. If you'd like to avoid the restrictions of the GPL and use Hoa Library for a closed-source product, you contact the <a title="CICM" href="http://cicm.mshparisnord.org/" target="_blank">CICM</a>.
