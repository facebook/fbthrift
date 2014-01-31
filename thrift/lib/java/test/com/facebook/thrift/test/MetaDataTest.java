/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


package com.facebook.thrift.test;

import java.util.Map;
import com.facebook.thrift.TFieldRequirementType;
import com.facebook.thrift.meta_data.FieldMetaData;
import com.facebook.thrift.meta_data.ListMetaData;
import com.facebook.thrift.meta_data.MapMetaData;
import com.facebook.thrift.meta_data.SetMetaData;
import com.facebook.thrift.meta_data.StructMetaData;
import com.facebook.thrift.protocol.TType;
import thrift.test.*;

public class MetaDataTest {

  public static void main(String[] args) throws Exception {
    Map<Integer, FieldMetaData> mdMap = CrazyNesting.metaDataMap;

    // Check for struct fields existence
    if (mdMap.size() != 3)
      throw new RuntimeException("metadata map contains wrong number of entries!");
    if (!mdMap.containsKey(CrazyNesting.SET_FIELD) || !mdMap.containsKey(CrazyNesting.LIST_FIELD) || !mdMap.containsKey(CrazyNesting.STRING_FIELD))
      throw new RuntimeException("metadata map doesn't contain entry for a struct field!");

    // Check for struct fields contents
    if (!mdMap.get(CrazyNesting.STRING_FIELD).fieldName.equals("string_field") ||
            !mdMap.get(CrazyNesting.LIST_FIELD).fieldName.equals("list_field") ||
            !mdMap.get(CrazyNesting.SET_FIELD).fieldName.equals("set_field"))
      throw new RuntimeException("metadata map contains a wrong fieldname");
    if (mdMap.get(CrazyNesting.STRING_FIELD).requirementType != TFieldRequirementType.DEFAULT ||
            mdMap.get(CrazyNesting.LIST_FIELD).requirementType != TFieldRequirementType.REQUIRED ||
            mdMap.get(CrazyNesting.SET_FIELD).requirementType != TFieldRequirementType.OPTIONAL)
      throw new RuntimeException("metadata map contains the wrong requirement type for a field");
    if (mdMap.get(CrazyNesting.STRING_FIELD).valueMetaData.type != TType.STRING ||
            mdMap.get(CrazyNesting.LIST_FIELD).valueMetaData.type != TType.LIST ||
            mdMap.get(CrazyNesting.SET_FIELD).valueMetaData.type != TType.SET)
      throw new RuntimeException("metadata map contains the wrong requirement type for a field");

    // Check nested structures
    if (!mdMap.get(CrazyNesting.LIST_FIELD).valueMetaData.isContainer())
      throw new RuntimeException("value metadata for a list is stored as non-container!");
    if (mdMap.get(CrazyNesting.LIST_FIELD).valueMetaData.isStruct())
      throw new RuntimeException("value metadata for a list is stored as a struct!");
    if (((MapMetaData)((ListMetaData)((SetMetaData)((MapMetaData)((MapMetaData)((ListMetaData)mdMap.get(CrazyNesting.LIST_FIELD).valueMetaData).elemMetaData).valueMetaData).valueMetaData).elemMetaData).elemMetaData).keyMetaData.type != TType.STRUCT)
      throw new RuntimeException("metadata map contains wrong type for a value in a deeply nested structure");
    if (((StructMetaData)((MapMetaData)((ListMetaData)((SetMetaData)((MapMetaData)((MapMetaData)((ListMetaData)mdMap.get(CrazyNesting.LIST_FIELD).valueMetaData).elemMetaData).valueMetaData).valueMetaData).elemMetaData).elemMetaData).keyMetaData).structClass != Insanity.class)
      throw new RuntimeException("metadata map contains wrong class for a struct in a deeply nested structure");

    // Check that FieldMetaData contains a map with metadata for all generated struct classes
    if (FieldMetaData.getStructMetaDataMap(CrazyNesting.class) == null ||
            FieldMetaData.getStructMetaDataMap(Insanity.class) == null ||
            FieldMetaData.getStructMetaDataMap(Xtruct.class) == null)
      throw new RuntimeException("global metadata map doesn't contain an entry for a known struct");
    if (FieldMetaData.getStructMetaDataMap(CrazyNesting.class) != CrazyNesting.metaDataMap ||
            FieldMetaData.getStructMetaDataMap(Insanity.class) != Insanity.metaDataMap)
      throw new RuntimeException("global metadata map contains wrong entry for a loaded struct");
  }
}
