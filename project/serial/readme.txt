xml, xsd validation website.
http://www.utilities-online.info/xsdvalidation/#.Vf2u9ZOVvts

getting started with xsdcxx
http://www.codesynthesis.com/projects/xsd/documentation/cxx/tree/guide/

command line switches for xsdcxx
http://www.codesynthesis.com/projects/xsd/documentation/xsd.xhtml

general note: I had better luck running commands from output directory and 'relative pathing' to input files.
opposed to trying to use the --output-dir option. This makes the includes for both input and output work.

generate parsing files.
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --output-dir xsdcxx projectXML.xsd

ok this is screwy. the parser, by default, validates the xml so it
needs the xsd at runtime. We have the xsd and a default xml in
the qt controlled resources. setup() creates these files in the temp
directory if they don't already exist.


//first one should be static so only run it if the file is missing.
cd */cadseer/source
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --generate-xml-schema xmlbase.xsd

//these will need to be run when the respective xsd file has been changed.
cd */cadseer/source/project/serial/xsdcxxoutput
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema ../../../xmlbase.xsd ../project.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema ../../../xmlbase.xsd ../featurebase.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featurecsysbase.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurecsysbase.xsd ../featurebox.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurecsysbase.xsd ../featurecylinder.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurecsysbase.xsd ../featuresphere.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurecsysbase.xsd ../featurecone.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema ../../../xmlbase.xsd ../featurebooleanbase.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebooleanbase.xsd ../featureunion.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebooleanbase.xsd ../featureintersect.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebooleanbase.xsd ../featuresubtract.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurecsysbase.xsd ../featureinert.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featureblend.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featurechamfer.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featuredraft.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featuredatumplane.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featurehollow.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL --extern-xml-schema featurebase.xsd ../featureoblong.xsd

