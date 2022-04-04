export interface IOverlay {
  // Component to render inside of overlay
  component: any;

  // Input field values
  inputs: { [key: string]: any }

  // Whether or not it's closeable through user-action
  userClosable: boolean;
}