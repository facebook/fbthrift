#!/bin/sh

mvn clean deploy -DaltDeploymentRepository=libs-snapshots-local::default::https://maven.thefacebook.com/nexus/content/repositories/libs-snapshots-local
