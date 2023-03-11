// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package service // [[[ program thrift source path ]]]

import (
  "fmt"

  module "module"
  includes "includes"
  "github.com/facebook/fbthrift/thrift/lib/go/thrift"
)

var _ = module.GoUnusedProtection__
var _ = includes.GoUnusedProtection__

// (needed to ensure safety because of naive import list construction)
var _ = fmt.Printf
var _ = thrift.ZERO


type IncludesIncluded = includes.Included

func NewIncludesIncluded() *IncludesIncluded {
  return includes.NewIncluded()
}

func WriteIncludesIncluded(item *IncludesIncluded, p thrift.Protocol) error {
  if err := item.Write(p); err != nil {
    return err
}
  return nil
}

func ReadIncludesIncluded(p thrift.Protocol) (IncludesIncluded, error) {
  var decodeResult IncludesIncluded
  decodeErr := func() error {
    result := *includes.NewIncluded()
err := result.Read(p)
if err != nil {
    return err
}
    decodeResult = result
    return nil
  }()
  return decodeResult, decodeErr
}

type IncludesTransitiveFoo = includes.TransitiveFoo

func NewIncludesTransitiveFoo() *IncludesTransitiveFoo {
  return includes.NewTransitiveFoo()
}

func WriteIncludesTransitiveFoo(item *IncludesTransitiveFoo, p thrift.Protocol) error {
  err := includes.WriteTransitiveFoo(item, p)
if err != nil {
    return err
}
  return nil
}

func ReadIncludesTransitiveFoo(p thrift.Protocol) (IncludesTransitiveFoo, error) {
  var decodeResult IncludesTransitiveFoo
  decodeErr := func() error {
    result, err := includes.ReadTransitiveFoo(p)
if err != nil {
    return err
}
    decodeResult = result
    return nil
  }()
  return decodeResult, decodeErr
}
