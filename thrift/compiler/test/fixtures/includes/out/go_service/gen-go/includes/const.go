// Autogenerated by Thrift for includes.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package includes

import (
    transitive "transitive"
    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

var _ = transitive.GoUnusedProtection__
// (needed to ensure safety because of naive import list construction)
var _ = thrift.ZERO

var GoUnusedProtection__ int

var ExampleIncluded *Included = NewIncluded().
    SetMyIntFieldNonCompat(2).
    SetMyTransitiveFieldNonCompat(
        *transitive.ExampleFoo,
    )
const IncludedConstant int64 = 42
