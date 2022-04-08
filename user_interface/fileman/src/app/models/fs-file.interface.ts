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

export const formatFileSize = (size: number): string => {
  const gib = size / (1024 ** 3);
  if (gib >= 1)
    return `${Math.round(gib * 100) / 100} GiB`;

  const mib = size / (1024 ** 2);
  if (mib >= 1)
    return `${Math.round(mib * 100) / 100} MiB`;

  const kib = size / (1024);
  if (kib >= 1)
    return `${Math.round(kib * 100) / 100} KiB`;

  return `${size} Bytes`;
}

export const createAndDownloadBlobFile = (
  contents: Uint8Array,
  filename: string,
  extension: string
) => {
  const blob = new Blob([contents]);
  const fileName = `${filename}.${extension}`;
  const url = URL.createObjectURL(blob);

  const link = document.createElement('a');
  link.setAttribute('href', url);
  link.setAttribute('download', fileName);
  link.style.visibility = 'hidden';

  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}