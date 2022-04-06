import { ISortableRowCell } from './sortable-row-cell.interface';

export interface ISortableRow {
  // Cell clicked callback
  clicked?: () => void;

  // All cells within this row, in same order as the columns
  cells: ISortableRowCell[];
}