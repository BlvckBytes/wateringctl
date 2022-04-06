import { ComponentRef, KeyValueDiffer } from '@angular/core';

export interface ISortableRowCell {
  // Cell's value
  value: any;

  // Whether or not the value is a component to be instantiated
  // True means yes, false means value is an immediate string value
  // Defaults to false
  isValueComponent?: boolean;

  // Component reference placeholder for instantiated components
  componentInstance?: ComponentRef<any>;

  // The wrapper where the instantiated component will be placed into
  componentWrapper?: HTMLDivElement;

  // Input field values for component instantiation
  inputs?: { [key: string]: any }

  // Differ used to diff the inputs to the component
  inputsDiffer?: KeyValueDiffer<string, any>;

  // Cell clicked callback
  clicked?: () => void;
}