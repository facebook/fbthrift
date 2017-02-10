package gotest

import "thrift/test/go/if/thrifttest"

type testHandler struct {
	ReturnError error
}

func (t *testHandler) DoTestVoid() error {
	return t.ReturnError
}

func (t *testHandler) DoTestString(thing string) (string, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestByte(thing int8) (int8, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestI32(thing int32) (int32, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestI64(thing int64) (int64, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestDouble(thing float64) (float64, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestFloat(thing float32) (float32, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestStruct(thing *thrifttest.Xtruct) (*thrifttest.Xtruct, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestNest(thing *thrifttest.Xtruct2) (*thrifttest.Xtruct2, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestMap(thing map[int32]int32) (map[int32]int32, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestSet(thing map[int32]bool) (map[int32]bool, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestList(thing []int32) ([]int32, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestEnum(thing thrifttest.Numberz) (thrifttest.Numberz, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestTypedef(thing thrifttest.UserId) (thrifttest.UserId, error) {
	return thing, t.ReturnError
}

func (t *testHandler) DoTestMapMap(hello int32) (map[int32]map[int32]int32, error) {
	res := map[int32]map[int32]int32{}
	for i := int32(0); i < hello; i++ {
		res[i] = map[int32]int32{i: i}
	}
	return res, t.ReturnError
}

func (t *testHandler) DoTestInsanity(argument *thrifttest.Insanity) (map[thrifttest.UserId]map[thrifttest.Numberz]*thrifttest.Insanity, error) {
	ret := map[thrifttest.UserId]map[thrifttest.Numberz]*thrifttest.Insanity{}
	ret[thrifttest.UserId(3)] = map[thrifttest.Numberz]*thrifttest.Insanity{
		thrifttest.Numberz_EIGHT: argument,
	}
	return ret, t.ReturnError
}

func (t *testHandler) DoTestMulti(
	arg0 int8, arg1 int32, arg2 int64, arg3 map[int16]string,
	arg4 thrifttest.Numberz, arg5 thrifttest.UserId,
) (*thrifttest.Xtruct, error) {
	xs := thrifttest.NewXtruct()
	xs.ByteThing = arg0
	xs.I32Thing = arg1
	xs.I64Thing = arg2
	return xs, t.ReturnError
}

func (t *testHandler) DoTestException(arg string) error {
	return t.ReturnError
}

func (t *testHandler) DoTestMultiException(arg0, arg1 string) (*thrifttest.Xtruct, error) {
	xs := thrifttest.NewXtruct()
	xs.StringThing = arg0
	return xs, t.ReturnError
}

func (t *testHandler) DoTestOneway(secondsToSleep int32) error {
	return t.ReturnError
}
