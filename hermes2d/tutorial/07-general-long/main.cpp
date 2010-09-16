#define H2D_REPORT_INFO
#define H2D_REPORT_FILE "application.log"
#include "hermes2d.h"

// This is a long version of example 07-general: function solve_linear() is not used.

const int P_INIT = 2;                             // Initial polynomial degree of all mesh elements.
const int INIT_REF_NUM = 3;                       // Number of initial uniform refinements.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_MUMPS, SOLVER_NOX, 
                                                  // SOLVER_PARDISO, SOLVER_PETSC, SOLVER_UMFPACK.

// Problem parameters.
double a_11(double x, double y) {
  if (y > 0) return 1 + x*x + y*y;
  else return 1;
}

double a_22(double x, double y) {
  if (y > 0) return 1;
  else return 1 + x*x + y*y;
}

double a_12(double x, double y) {
  return 1;
}

double a_21(double x, double y) {
  return 1;
}

double a_1(double x, double y) {
  return 0.0;
}

double a_2(double x, double y) {
  return 0.0;
}

double a_0(double x, double y) {
  return 0.0;
}

double rhs(double x, double y) {
  return 1 + x*x + y*y;
}

double g_D(double x, double y) {
  return -cos(M_PI*x);
}

double g_N(double x, double y) {
  return 0;
}

// Boundary condition types.
BCType bc_types(int marker)
{
  if (marker == 1) return BC_ESSENTIAL;
  else return BC_NATURAL;
}

// Essential (Dirichlet) boundary condition values.
scalar essential_bc_values(int ess_bdy_marker, double x, double y)
{
  return g_D(x, y);
}

// Weak forms.
#include "forms.cpp"

int main(int argc, char* argv[])
{
  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("domain.mesh", &mesh);

  // Perform initial mesh refinements.
  for (int i=0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();

  // Create an H1 space with default shapeset.
  H1Space space(&mesh, bc_types, essential_bc_values, P_INIT);
  int ndof = get_num_dofs(&space);
  info("ndof = %d", ndof);

  // Initialize the weak formulation.
  WeakForm wf;
  wf.add_matrix_form(bilinear_form, bilinear_form_ord, H2D_SYM);
  wf.add_vector_form(linear_form, linear_form_ord);
  wf.add_vector_form_surf(linear_form_surf, linear_form_surf_ord, 2);

  // Initialize the linear problem.
  bool is_linear = true;
  FeProblem fep(&wf, &space, is_linear);

  // Initialize matrix solver.
  Solver* solver;
  switch (matrix_solver) {
    case SOLVER_AMESOS: solver = new AmesosSolver("Amesos_Klu", &fep); info("Using Amesos"); break;
    case SOLVER_MUMPS: solver = new MumpsSolver(&fep); info("Using Mumps"); break;
    case SOLVER_NOX: solver = new NoxSolver(&fep); info("Using Nox"); break;
    case SOLVER_PARDISO: solver = new PardisoLinearSolver(&fep); info("Using Pardiso"); break;
    case SOLVER_PETSC: solver = new PetscLinearSolver(&fep); info("Using PETSc"); break;
    case SOLVER_UMFPACK: solver = new UMFPackLinearSolver(&fep); info("Using UMFPack"); break;
    default: error("Unknown matrix solver requested.");
  }

  // Solve the matrix problem.
  if (!solver->solve()) error ("Matrix solver failed.\n");

  // Extract solution vector.
  scalar* coeffs = solver->get_solution();

  // Convert coefficient vector into a Solution.
  Solution* sln = new Solution(&space, coeffs);

  // Destroy matrix solver.
  delete solver;

  // Time measurement.
  cpu_time.tick();

  // View the solution and mesh.
  ScalarView sview("Coarse solution", new WinGeom(0, 0, 440, 350));
  sview.show(sln);
  OrderView  oview("Polynomial orders", new WinGeom(450, 0, 400, 350));
  oview.show(&space);

  // Skip visualization time.
  cpu_time.tick(HERMES_SKIP);

  // Print timing information.
  verbose("Total running time: %g s", cpu_time.accumulated());

  // Wait for all views to be closed.
  View::wait();
  return 0;
}
