const {Solver} = require('.')

const fs = require('fs')
function parseDimacsProblem(str) {
  const lines = str.split(/\r?\n/)
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i]
    if (line.startsWith("c ")) continue
    if (line.startsWith("p ")) {
      const problem = /^p\s+cnf\s+(\d+)\s+(\d+)/.exec(line)
      if (!problem) throw new Error("Didn't understand problem line")
      const numVariables = Number(problem[1])
      const numClauses = Number(problem[2])
      const clauses = []
      for (let j = i + 1; j < lines.length; j++) {
        if (lines[j].startsWith("c ")) continue
        const clause = lines[j].trim().split(/\s+/).map(x => Number(x))
        clause.length -= 1 // drop the final zero
        if (clause.length)
          clauses.push(clause)
      }
        /*
      if (clauses.length !== numClauses)
        throw new Error(`clause count does not match ${numClauses} != ${clauses.length}`)
        */
      return { numVariables, clauses }
    }
  }
  throw new Error("No problem line found")
}

function containsRepeatedValues(xs) {
  return (new Set(xs)).size < xs.length
}

function checkSolution(problem, solution) {
  if (!solution.satisfiable)
    throw new Error("I don't know how to check proofs of unsatisfiability, for that you'll need DRAT")
  if (containsRepeatedValues(solution.assignments.map(x => Math.abs(x))))
    throw new Error("Solution contained multiple assignments for the same variable")
  const assignments = new Set(solution.assignments)
  const unsatisifedClause = problem.clauses.find(clause => !clause.some(literal => assignments.has(literal)))
  if (unsatisifedClause)
    throw new Error(`A clause wasn't satisfied: ${unsatisifedClause}`)
}

function exportAsDimacs(prob) {
  let lines = []
  lines.push(`p cnf ${prob.numVariables} ${prob.clauses.length}`)
  for (const clause of prob.clauses) {
    lines.push(clause.join(' ') + ' 0')
  }

  return lines.join('\n')
}

let solved = 0
let cpuTime = 0n
;(async () => {
for (const problemFileName of process.argv.slice(2)) {
  console.log(problemFileName)
  const prob = parseDimacsProblem(fs.readFileSync(problemFileName, 'ascii'))

  const s = new Solver

  for (let i = 0; i < prob.numVariables; i++)
    s.newVar()

  for (const clause of prob.clauses) {
    s.addClause(clause)
  }

  const begin = process.hrtime.bigint()
  //setTimeout(() => s.interrupt(), 1000)
  let res;
  if (s.simplify())
    res = await s.solve()
  cpuTime += process.hrtime.bigint() - begin

  if (res !== undefined && s.okay()) {
    const assignments = []
    for (let v = 1; v <= prob.numVariables; v++) {
      const val = s.value(v)
      if (val === true)
        assignments.push(v)
      else if (val === false)
        assignments.push(-v)
    }
    const solution = {
      satisfiable: true,
      assignments
    }
    console.log(solution)
    checkSolution(prob, solution)
  }
  solved += 1

}
})().then(() => {
  console.log(`solved ${solved} instances in ${(Number(cpuTime) / 1e6).toFixed(2)} ms`)
})
