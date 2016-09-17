#!/bin/sh

mvn clean deploy -DaltDeploymentRepository=libs-snapshots-local::default::http://nexus.vip.facebook.com:8181/nexus/content/repositories/libs-snapshots-local
