namespace cpp tutorial.sort

exception SortError {
  1: string msg
}

/**
 * Service for sorting a list of integers
 */
service Sorter {
  /**
   * Sort a list of integers
   */
  list<i32> sort(1: list<i32> values) throws (1: SortError sort_error);
}
