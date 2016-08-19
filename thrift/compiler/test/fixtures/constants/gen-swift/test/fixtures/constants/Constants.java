package test.fixtures.constants;

import com.facebook.swift.codec.*;
import com.google.common.collect.*;
import java.util.*;

public final class Constants
{
    private Constants() {
    }

    public static final String apostrophe = "'";

    public static final String backslash = "\\";

    public static final Map<String, Integer> char2ascii = ImmutableMap.<String, Integer>builder()
        .put("'", 39)
        .put(""", 34)
        .put("\\", 92)
        .put("\x61", 97)
        .build();

    public static final Map<Integer, Integer> empty_int_int_map = ImmutableMap.<Integer, Integer>builder()
        .build();

    public static final List<Integer> empty_int_list = ImmutableList.<Integer>builder()
        .build();

    public static final Set<Integer> empty_int_set = ImmutableSet.<Integer>builder()
        .build();

    public static final Map<Integer, String> empty_int_string_map = ImmutableMap.<Integer, String>builder()
        .build();

    public static final String empty_string = "";

    public static final Map<String, Integer> empty_string_int_map = ImmutableMap.<String, Integer>builder()
        .build();

    public static final List<String> empty_string_list = ImmutableList.<String>builder()
        .build();

    public static final Set<String> empty_string_set = ImmutableSet.<String>builder()
        .build();

    public static final Map<String, String> empty_string_string_map = ImmutableMap.<String, String>builder()
        .build();

    public static final String escaped_a = "\x61";

    public static final List<String> escaped_strings = ImmutableList.<String>builder()
        .add("\x61")
        .add("\xab")
        .add("\x6a")
        .add("\xa6")
        .add("\x61yyy")
        .add("\xabyyy")
        .add("\x6ayyy")
        .add("\xa6yyy")
        .add("zzz\x61")
        .add("zzz\xab")
        .add("zzz\x6a")
        .add("zzz\xa6")
        .add("zzz\x61yyy")
        .add("zzz\xabyyy")
        .add("zzz\x6ayyy")
        .add("zzz\xa6yyy")
        .build();

    public static final boolean false_c = false;

    public static final test.fixtures.constants.Internship instagram = ;

    public static final List<test.fixtures.constants.Internship> internList = ImmutableList.<test.fixtures.constants.Internship>builder()
        .add()
        .add()
        .build();

    public static final List<test.fixtures.constants.Range> kRanges = ImmutableList.<test.fixtures.constants.Range>builder()
        .add()
        .add()
        .build();

    public static final int myInt = 1337;

    public static final String name = "Mark Zuckerberg";

    public static final String quotationMark = """;

    public static final List<Map<String, Integer>> states = ImmutableList.<Map<String, Integer>>builder()
        .add(ImmutableMap.<String, Integer>builder()
        .put("San Diego", 3211000)
        .put("Sacramento", 479600)
        .put("SF", 837400)
        .build())
        .add(ImmutableMap.<String, Integer>builder()
        .put("New York", 8406000)
        .put("Albany", 98400)
        .build())
        .build();

    public static final String tripleApostrophe = "'''";

    public static final boolean true_c = true;

    public static final double x = 1.000000;

    public static final double y = 1000000.0;

    public static final double z = 1000000000.000000;

    public static final short zero16 = 0;

    public static final int zero32 = 0;

    public static final long zero64 = 0L;

    public static final byte zero_byte = 0;

    public static final double zero_dot_zero = 0.000000;
}
