#!/bin/sh

#build jar and sources.jar
mvn clean package

#auto generate version
export AUTO_ARTIFACT_VERSION=`./generate-git-version.pl`

#generate pom.xml with auto versioning
cat pom.xml | sed -e "s/<thrift.version>.*<\/thrift.version>/<thrift.version>$AUTO_ARTIFACT_VERSION<\/thrift.version>/" > target/pom.xml

export JAR=$(ls target/thrift-*.jar|grep -v sources)
export SOURCE_JAR=$(ls target/thrift-*.jar|grep sources)

export CMD="mvn org.apache.maven.plugins:maven-deploy-plugin:2.7:deploy-file \
  -DgroupId=com.facebook -DartifactId=thrift -Dversion=$AUTO_ARTIFACT_VERSION -Dpackaging=jar \
  -Dfile=$JAR -Dsource=$SOURCE_JAR -DpomFile=target/pom.xml \
  -Durl=http://nexus.vip.facebook.com:8181/nexus/content/repositories/libs-releases-local \
  -DrepositoryId=libs-releases-local"

echo executing $CMD
echo

$CMD

