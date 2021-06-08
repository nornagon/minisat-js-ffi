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
      const numVariables = parseInt(problem[1])
      const numClauses = parseInt(problem[2])
      const clauses = []
      for (let j = i + 1; j < i + 1 + numClauses; j++) {
        // TODO: can there be comments mid-problem?
        const clause = lines[j].trim().split(/\s+/).map(x => parseInt(x))
        clause.length -= 1 // drop the final zero
        clauses.push(clause)
      }
      return { numVariables, clauses }
    }
  }
  throw new Error("No problem line found")
}

let solved = 0
let cpuTime = 0n
for (const problemFileName of process.argv.slice(2)) {
  const prob = parseDimacsProblem(fs.readFileSync(problemFileName, 'ascii'))

  const s = new Solver

  for (let i = 0; i < prob.numVariables; i++)
    s.newVar()

  for (const clause of prob.clauses) {
    s.addClause(clause.map(x => (Math.abs(x) - 1) * (x > 0 ? 1 : -1)))
  }


  const begin = process.hrtime.bigint()
  if (s.simplify())
    s.solve()
  solved += 1
  cpuTime += process.hrtime.bigint() - begin
}
console.log(`solved ${solved} instances in ${(Number(cpuTime) / 1e6).toFixed(2)} ms`)
