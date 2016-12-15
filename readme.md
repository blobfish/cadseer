# cadseer

A parametric solid modeler.

## Getting Started

These instructions may help you get cadseer running on your system.

### Prerequisites

* Qt4 REQUIRED COMPONENTS QtCore QtGui
* Boost REQUIRED COMPONENTS system graph
* OpenCasCade REQUIRED
* OpenSceneGraph REQUIRED
* Eigen3 REQUIRED
* XercesC REQUIRED
* XSDCXX REQUIRED


### Installing

Git (pun intended) a copy of the repository.

```
git clone --recursive -j8 https://gitlab.com/blobfish/cadseer.git cadseer
```

Setup for build.

```
cd cadseer
mkdir build
cd build
```

Build

```
cmake -DCMAKE_BUILD_TYPE:STRING=Debug ..
make -j4
```

## Built With

* [OpenCasCade](https://www.opencascade.com/) - Solid modeling kernel
* [OpenSceneGraph](http://www.openscenegraph.org/) - OpenGL visualization
* [Qt](https://www.qt.io/) - GUI
* [Boost](http://www.boost.org/) - C++ library
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) - C++ linear algebra library
* [XSDCXX](http://www.codesynthesis.com/products/xsd/) - C++ xml parsing library

## License

This project is licensed under the GPLv3 License
