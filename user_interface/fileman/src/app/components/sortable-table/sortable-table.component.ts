import { ApplicationRef, Component, ComponentFactoryResolver, DoCheck, EmbeddedViewRef, Injector, Input, IterableDiffer, IterableDiffers, KeyValueDiffers, OnChanges, OnDestroy, OnInit, SimpleChanges } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { IStatePersistable } from 'src/app/models/state-persistable.interface';
import { ComponentStateService } from 'src/app/services/component-state.service';
import { SortDirection } from './sort-direction.type';
import { ISortableColumn } from './sortable-column.interface';
import { ISortableRowCell } from './sortable-row-cell.interface';
import { ISortableRow } from './sortable-row.interface';

@Component({
  selector: 'app-sortable-table',
  templateUrl: './sortable-table.component.html',
  styleUrls: ['./sortable-table.component.scss']
})
export class SortableTableComponent implements OnDestroy, DoCheck, OnInit, IStatePersistable<ISortableColumn<any>[]> {

  /*
  ============================================================================
                               Private Properties                             
  ============================================================================
  */

  private _columns: ISortableColumn<any>[] = [];               // Table columns currently displayed
  private _rows: ISortableRow[] = [];                     // Table rows currently displayed
  private _rowsDiffer?: IterableDiffer<ISortableRow>;     // Differ used to detect row changes
  private _state?: ISortableColumn<any>[];                     // Current column state
  private _rowSortedIndices: number[] = [];               // Indices of _rows, in sorted order

  /*
  ============================================================================
                                Public Properties                             
  ============================================================================
  */

  get columns(): ISortableColumn<any>[] {
    return this._columns;
  }

  // Apply sorting based on sorted indices
  get rows(): ISortableRow[] {
    const sorted: ISortableRow[] = [];
    this._rowSortedIndices.forEach(i => sorted.push(this._rows[i]));
    return sorted;
  }

  set state(v: ISortableColumn<any>[]) {
    this._state = v;
    this.applyColumnState();
  }

  get state(): ISortableColumn<any>[] {
    // Shave off properties that don't need any persistence from the state-clone
    this._state = this._columns.map(it => <ISortableColumn<any>>{
      name: it.name,
      currentDirection: it.currentDirection,
    });
    return this._state;
  }

  stateToken?: string; // State token used by the state service

  /*
  ============================================================================
                                     Inputs                                   
  ============================================================================
  */

  @Input() markEmpty?: string;           // Text of the row that marks an empty table

  // Token to use for persisting the sort state
  @Input() set stateTokenDesc(v: string) {
    this.stateToken = `sortable-table--${v}`;
  }

  @Input()
  set columns(value: ISortableColumn<any>[]) {
    // Set default direction value if applicable
    value.forEach(v => (v.currentDirection = v.currentDirection || 'none'));

    // Load and apply the current known state, where possible
    this._columns = value;
    this.applyColumnState();
  }

  @Input()
  set rows(value: ISortableRow[]) {
    // Create list differ
    if (value)
      this._rowsDiffer = this.itDiffer.find(value).create();

    // Instantiate all cell components that still need to be created
    value.forEach(v => v.cells.forEach(c => this.instantiateComponent(c)));
    this._rows = value;

    // Sort
    this.generateRowSortedIndices();
  }

  /*
  ============================================================================
                             Dependency Injection                             
  ============================================================================
  */

  constructor(
    private tranService: TranslateService,
    private compFacRes: ComponentFactoryResolver,
    private inj: Injector,
    private appRef: ApplicationRef,
    private itDiffer: IterableDiffers,
    private kvDiffer: KeyValueDiffers,
    private stateService: ComponentStateService,
  ) {}

  /*
  ============================================================================
                                    Lifecycles                                
  ============================================================================
  */

  ngOnInit(): void {
    this.stateService.load(this);
  }

  ngDoCheck(): void {
    for (const row of this._rows) {
      for (const cell of row.cells) {
        // Differ not initialized or cell is no component or has no inputs
        if (!cell.inputsDiffer || !cell.isValueComponent || !cell.inputs)
          continue;

        // Re-apply the inputs if there are any changes
        if (cell.inputsDiffer.diff(cell.inputs))
          this.applyInputs(cell);
      }
    }

    // Differ not initialized
    if (this._rowsDiffer) {
      
      // check if options have changed, apply
      const changes = this._rowsDiffer.diff(this._rows);
      if (changes) {

        // Check and create all needed components for this new row
        changes.forEachAddedItem(i => {
          i.item.cells.forEach(c => this.instantiateComponent(c));
        });

        // Destroy all created components for this removed row
        changes.forEachRemovedItem(i => {
          i.item.cells.forEach(c => this.destroyCell(c));
        });

        // Sort
        this.generateRowSortedIndices();
      }
    }
  }

  ngOnDestroy(): void {
    // Destroy all cells on self-destruct
    this._rows.forEach(r => r.cells.forEach(c => this.destroyCell(c)));
  }

  /*
  ============================================================================
                                   Utilities                                  
  ============================================================================
  */

  private applyColumnState() {
    if (!this._state)
      return;

    // Create a lookup table from the state for faster access
    const stateLUT = this._state
      .reduce((acc, curr) => {
        acc[curr.name] = curr.currentDirection || 'none';
        return acc;
      }, <{ [key: string]: SortDirection }>{});

    // Patch all columns
    this._columns.forEach(c => {
      c.currentDirection = stateLUT[c.name] || c.currentDirection;
    });
  }

  private destroyCell(cell: ISortableRowCell) {
    // Not a component
    if (!cell.componentInstance)
      return;

    // Detatch and destroy
    this.appRef.detachView(cell.componentInstance.hostView);
    cell.componentInstance.destroy();
  }
  
  private applyInputs(cell: ISortableRowCell) {
    // No inputs provided or not yet instantiated
    if (!cell.inputs || !cell.componentInstance)
      return;

    // Apply the inputs
    const inst = cell.componentInstance.instance;
    for (const key of Object.keys(cell.inputs)) {
      // Only apply if the input object truly has this property (not in prototype chain)
      // and if the target component has a corresponding field to apply to
      if (inst.hasOwnProperty(key) && cell.inputs.hasOwnProperty(key))
        inst[key] = cell.inputs[key];
    }
  }

  private instantiateComponent(cell: ISortableRowCell) {
    // Not a component or already instantiated
    if (!cell.isValueComponent || cell.componentInstance)
      return;

    // Create instance of component
    const fac = this.compFacRes.resolveComponentFactory(cell.value);
    const comp = fac.create(this.inj);

    // Save component instance
    cell.componentInstance = comp;

    // Apply inputs if provided
    if (cell.inputs) {
      // Generate kv differ for the inputs object
      cell.inputsDiffer = this.kvDiffer.find(cell.inputs).create();

      // Initially apply the inputs
      this.applyInputs(cell);
    }

    // Attach to view
    this.appRef.attachView(comp.hostView);
  }

  private compareRows(a: ISortableRow, b: ISortableRow): number {

    let colIndex = 0;
    let result = 0;

    // Loop all columns
    while(
      result === 0 &&                     // While the current result means "equal"
      (colIndex < this._columns.length)   // And there are still columns left
    ) {
      // Get currently pointed at column
      const currIndex = colIndex++;
      const col = this._columns[currIndex];

      // This column has no effect on sorting
      if (col.currentDirection === 'none')
        continue;

      // Cannot go any further, as any of the two rows doesn't
      // seem to have all rows implemented
      if (
        currIndex >= a.cells.length ||
        currIndex >= b.cells.length
      )
        break;

      // Get column-pointer target cells' value
      let aCell = a.cells[currIndex];
      let bCell = b.cells[currIndex];

      // Either get the immediate string value or the instance ref
      let aVal = aCell.isValueComponent ? aCell.componentInstance?.instance : aCell.value;
      let bVal = bCell.isValueComponent ? bCell.componentInstance?.instance : bCell.value;

      // No special sorter provided, sort using text-compare
      const sorter = col.sortFn;
      if (sorter === undefined)
        result = String(aVal).localeCompare(String(bVal));
      
      // Sort using the specified sort function
      else
        result = sorter(aVal, bVal);

      // Invert result
      if (col.currentDirection === 'desc')
        result *= -1;
    }

    return result;
  }

  private generateRowSortedIndices() {
    this._rowSortedIndices = [...Array(this._rows.length).keys()]
      .sort((a, b) => {
        const aRow = this._rows[a];
        const bRow = this._rows[b];
        return this.compareRows(aRow, bRow);
      });
  }

  resolveColumnName(column: ISortableColumn<any>): string {
    if (column.translate)
      return this.tranService.instant(column.name, column.translateArgs);
    return column.name;
  }

  renderComponent(cell: ISortableRowCell, wrapper: HTMLDivElement) {
    // Nothing to render
    if (!cell.componentInstance)
      return;

    const elem = (cell.componentInstance.hostView as EmbeddedViewRef<any>).rootNodes[0] as HTMLElement;
    wrapper.appendChild(elem);
  }

  cycleColumnSort(column: ISortableColumn<any>) {
    if (column.currentDirection == 'none')
      column.currentDirection = 'asc';
    else if (column.currentDirection == 'asc')
      column.currentDirection = 'desc';
    else if (column.currentDirection == 'desc')
      column.currentDirection = 'none';

     // Persist sorting state
    this.stateService.save(this);

    // Sort
    this.generateRowSortedIndices();
  }

  private columnByIndex(columnIndex: number): ISortableColumn<any> | null {
    // Didn't provide all required columns
    if (columnIndex >= this._columns.length - 1)
      return null;
    return this._columns[columnIndex];
  }

  resolveCellValue(columnIndex: number, cell: ISortableRowCell): string {
    // No renderer provided
    const renderer = this.columnByIndex(columnIndex)?.renderFn;
    if (!renderer)
      return cell.value;

    // Run through renderer
    return renderer(cell.value);
  }

  isCellBreakAll(columnIndex: number): boolean {
    return this.columnByIndex(columnIndex)?.break || false;
  }

  isCellSelectable(columnIndex: number): boolean {
    const value = this.columnByIndex(columnIndex)?.selectable;

    if (value === undefined)
      return true;

    return value;
  }

  cellClicked(cell: ISortableRowCell, e: MouseEvent) {
    // No click callback set
    if (!cell.clicked)
      return

    // Stop propagation to avoid triggering the parent (row) callback
    e.stopPropagation();
    cell.clicked();
  }
}
