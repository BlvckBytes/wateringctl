export interface IFSFile {
  isDirectory: boolean;
  size: number;
  name: string;
}

export const getFileName = (name: string): string => {
  // No slash (should never occur)
  if (!name.includes('/'))
    return name;

  // Get substring after last slash
  return name.substring(name.lastIndexOf('/') + 1);
}
