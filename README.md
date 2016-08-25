##An R-tree powered MongoDB##

Longgang XIANG  (Project leader), Xiaotian SHAO (Technical member), Dehao Wang (Technical member)

State Key laboratory of Information Engineering in Mapping, Surveying and Remote Sensing, Wuhan University, Wuhan 430079, China

39334073@qq.com, 386520154@qq.com, 262730936@qq.com

######&#160; &#160; &#160; &#160;######

Welcome to MongoDB with R-tree suppport! In this branch, we develop an R-tree module and plug it into MongoDB. With the R-tree, you could manage planar spatial data, which is still widely used in geographical information systems, particularly in the city/country scale. To reduce the learning cost, the command interface supported by 2dsphere, like find, insert and update, is reused as much as possible. It means you could use the same insert/finde/update/remove command to populate/fetch/modify/delete planar spatial data. However, one additional command, i.e., registerGeometry, is introduced to register geometries taking layer as the basci unit. Besides, the create command is overridden to accept the parameters of R-tree, like branching-factor. See "Brief introduction.docx" to learn more about our R-tree implementation, and “Experimental evaluations.docx” to obtain the performance information of R-tree module

###Third-party library###

The GEOS library is explored for geometry calculation.

###R-tree demonstration###

An application with GUI is included in this branch to demonstrae the function of R-tree,in which both source codes and compiled binaries are provided! With it, you could import projected spatial data of point, line or polygon type, and then browse them as well as query them with three kinds of built-in filters, i.e., geowithin, geointersect and geonear. For more information, please consult "./demonstration/README.MD".

###How to build###

If you want to build the R-tree powered MongoDB, assure the following components are installed in your computer.

* a modern C++ compiler:
 - GCC 4.8.2 or newer
 - Visual Studio 2013 Update 2 or newer
 - Clang 3.4 (or Apple XCode 5.1.1 Clang) or newer
* Python 2.7
* SCons 2.3

For more information, please consult "./docx/building.md"

The uploaded source codes have been successfully compiled with following platforms (Other platforms may work but not tested):

    OS                    Status  Compiler

    Window 10, 64Bit      Pass    VS 2013 update 5

    Ubuntu 14.04, 64Bit   Pass    GCC 4.8.4, the defult compiler

    Ubuntu 16.04,64Bit    N/A  Failed with default GCC compiler, should use a lower version(e.g gcc 4.8.5)

###Acknowledement###

This project is carried out under the support of [Collaborative Innovation Center Of Geospatial Technology](http://innogst.whu.edu.cn/en/). We also thank [Wuda Geoinformatics Co., Ltd.](http://www.geostar.com.cn/index.php?l=english) for financial support.

####&#160; &#160; &#160; &#160;####
##Original README about MongoDB##

MongoDB README

Welcome to MongoDB!

COMPONENTS

* mongod - The database process.
* mongos - Sharding controller.
* mongo  - The database shell (uses interactive javascript).

UTILITIES

* mongodump         - MongoDB dump tool - for backups, snapshots, etc.
* mongorestore      - MongoDB restore a dump
* mongoexport       - Export a single collection to test (JSON, CSV)
* mongoimport       - Import from JSON or CSV
* mongofiles        - Utility for putting and getting files from MongoDB GridFS
* mongostat         - Show performance statistics

BUILDING

&#160; &#160; &#160; &#160;See docs/building.md, also www.mongodb.org search for "Building".

RUNNING

&#160; &#160; &#160; &#160;For command line options invoke:

    $ ./mongod --help

&#160; &#160; &#160; &#160;To run a single server database:

    $ mkdir /data/db
    $ ./mongod
    $
    $ # The mongo javascript shell connects to localhost and test database by default:
    $ ./mongo 
    > help

DRIVERS

&#160; &#160; &#160; &#160;Client drivers for most programming languages are available at mongodb.org.  Use the shell ("mongo") for administrative tasks.

PACKAGING

&#160; &#160; &#160; &#160;Packages are created dynamically by the package.py script located in the buildscripts directory. This will generate RPM and Debian packages.

DOCUMENTATION

&#160; &#160; &#160; &#160;http://www.mongodb.org/
 
CLOUD MANAGED MONGODB

&#160; &#160; &#160; &#160;http://cloud.mongodb.com/

MAIL LISTS AND IRC

&#160; &#160; &#160; &#160;http://dochub.mongodb.org/core/community
  
LEARN MONGODB

&#160; &#160; &#160; &#160;http://university.mongodb.com/

32 BIT BUILD NOTES

&#160; &#160; &#160; &#160;MongoDB uses memory mapped files.  If built as a 32 bit executable, you will not be able to work with large (multi-gigabyte) databases.  However, 32 bit builds work fine with small development databases.

LICENSE

&#160; &#160; &#160; &#160;Most MongoDB source files (src/mongo folder and below) are made available under the terms of the GNU Affero General Public License (AGPL). See individual files for details. 

&#160; &#160; &#160; &#160;As an exception, the files in the client/, debian/, rpm/, utils/mongoutils, and all subdirectories thereof are made available under the terms of the Apache License, version 2.0. 