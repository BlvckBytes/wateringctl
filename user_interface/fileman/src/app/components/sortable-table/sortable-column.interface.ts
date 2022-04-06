import { SortDirection } from './sort-direction.type';

export interface ISortableColumn<T> {
  // Column name
  name: string;

  // Whether or not to set word-break to break-all on cells
  // residing in this columns (used for long continuous values)
  break?: boolean;

  // Whether or not to make this cell's text-value selectable
  selectable?: boolean;

  // Function used to render cell values to a string (when values
  // are more complex values, more convenient for sorting or other purposes)
  // This will only be used when the cell is not a component
  renderFn?: (value: T) => string;

  // Whether or not the name is a translation identifier, defaults to false
  translate?: boolean;

  // Arguments used for translation interpolation
  translateArgs?: any;

  // Current sortding direction, defaults to none
  currentDirection?: SortDirection;

  // Whether or not this column is sortable, defaults to false
  sortable?: boolean;

  // Function used to compare two elements residing in this column
  // When it's not provided, basic string comparison will take place
  sortFn?: (a: T, b: T) => number;
}