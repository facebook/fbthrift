namespace py thrift.compiler.test.fixtures.json_experimental.src.DemoWidgetDocs
namespace wiki Thrift.DemoWidgetDocs

 /**
  * What kinds of widgets can we buy and sell?
  */
enum WidgetType {
  /**  Any small device or object (usually hand-sized) which can be manipulated.
  */
  FROB = 1
  /** The kind that you can use to open a door. */
  KNOB = 2
  /** An actual person named Bob. */
  BOB  = 3
}

/**
 * We are looking to buy / sell some quantity of widgets,
 * subject to price and quantity constraints.
 *
 * @should min(abs(minWidgets), abs(maxWidgets)) * minPrice <= budget,
 *         'Your requisition can never be satisfied'
 */
struct WidgetRequisition {
  1: WidgetType type
  /**
    How much can we spend on this order of widgets?
    @must _ > 0
   */
  2: i32 budgetCents
  /** Negative quantities represent sale requisitions. */
  3: i32 minWidgets
  /** @must minWidgets <= _ */
  4: i32 maxWidgets
  /** A lower limit on the price makes sense if our logistics are not set
      up to handle massive quantities of cheap stuff.
      @must _ >= 0
   */
  5: i32 minPriceCents = 0
  /** Our physical security can't deal with high-value items.
      @must minPrice <= _
   */
  6: i32 maxPriceCents
}

/** Once a WidgetRequisition is fulfilled, it becomes an order.

    @must abs(numWidgets) * priceCents <= requisition.budget, 'Over budget'
    @must requisition.minWidgets <= numWidgets and
          numWidgets <= requisition.maxWidgets
    @must requisition.minPriceCents <= priceCents and
          priceCents <= requisition.maxPriceCents
 */
struct WidgetOrder {
  /** The requisition that generated this order. */
  1: WidgetRequisition requisition
  /** Negative quantities represent sale orders. */
  2: i32 numWidgets
  /** @must _ >= 0 */
  3: i32 priceCents
}
