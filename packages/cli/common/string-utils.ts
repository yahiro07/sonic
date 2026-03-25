export function casingToCapital(str: string) {
  return (
    str.charAt(0).toUpperCase() +
    str.slice(1).replace(/[_-]([a-z])/g, (match) => match[1].toUpperCase())
  );
}

export function casingToSnake(str: string) {
  return str
    .replace(/([A-Z])/g, "_$1")
    .toLowerCase()
    .replace(/^_/, "");
}

export function casingToKebab(str: string) {
  return str
    .replace(/([A-Z])/g, "-$1")
    .toLowerCase()
    .replace(/^-/, "");
}

export function incrementSuffix(str: string) {
  const match = str.match(/(\d+)$/);
  if (match) {
    return str.replace(match[0], (parseInt(match[0], 10) + 1).toString());
  }
  return `${str}1`;
}

const lettersSource = {
  alpha: "abcdefghijklmnopqrstuvwxyz",
  alphaNumeric: "abcdefghijklmnopqrstuvwxyz0123456789",
  alphaNumericWithCapital:
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
};
export function generateRandomString(
  sourceKind: "alpha" | "alphaNumeric" | "alphaNumericWithCapital",
  len: number,
): string {
  const letters = lettersSource[sourceKind];
  return new Array(len)
    .fill(0)
    .map(() => letters[Math.floor(Math.random() * letters.length)])
    .join("");
}
