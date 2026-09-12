import { DEMOS, solveCnf } from "./dpll.js";

const input = document.getElementById("cnf") as HTMLTextAreaElement;
const out = document.getElementById("out") as HTMLPreElement;
const runBtn = document.getElementById("run") as HTMLButtonElement;
const demoSelect = document.getElementById("demo") as HTMLSelectElement;

function render() {
  try {
    const r = solveCnf(input.value);
    let text = `s ${r.status === "SAT" ? "SATISFIABLE" : "UNSATISFIABLE"}\n`;
    if (r.model) text += `v ${r.model.join(" ")} 0\n`;
    text += `c decisions=${r.decisions} propagations=${r.propagations}\n`;
    text += `c engine=browser-DPLL (C++ CDCL is the main satforge binary)\n`;
    out.textContent = text;
    out.dataset.status = r.status;
  } catch (e) {
    out.textContent = `error: ${(e as Error).message}`;
    out.dataset.status = "ERROR";
  }
}

runBtn.addEventListener("click", render);
demoSelect.addEventListener("change", () => {
  const key = demoSelect.value;
  if (key && DEMOS[key]) {
    input.value = DEMOS[key];
    render();
  }
});

input.value = DEMOS.sat_simple;
render();
