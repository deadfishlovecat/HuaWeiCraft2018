#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <locale.h>
#include "svm.h"

#define INF HUGE_VAL
#define TAU 1e-12

static void print_string_stdout(const char *s)
{
	fputs(s,stdout);
	fflush(stdout);
}
static void (*svm_print_string) (const char *) = &print_string_stdout;
#if 1
static void info(const char *fmt,...)
{
	char buf[BUFSIZ];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(buf,fmt,ap);
	va_end(ap);
	(*svm_print_string)(buf);
}
#else
static void info(const char *fmt,...) {}
#endif

//
// Kernel Cache
//
// l is the number of total data items
// size is the cache size limit in bytes
//
class Cache
{
public:
	Cache(int l,long int size);
	~Cache();

	// request data [0,len)
	// return some position p where [p,len) need to be filled
	// (p >= len if nothing needs to be filled)
	int get_data(const int index, float **data, int len);
	void swap_index(int i, int j);
private:
	int l;
	long int size;
	struct head_t
	{
		head_t *prev, *next;	// a circular list
		float *data;
		int len;		// data[0,len) is cached in this entry
	};

	head_t *head;
	head_t lru_head;
	void lru_delete(head_t *h);
	void lru_insert(head_t *h);
};

Cache::Cache(int l_,long int size_):l(l_),size(size_)
{
	head = (head_t *)calloc(l,sizeof(head_t));	// initialized to 0
	size /= sizeof(float);
	size -= l * sizeof(head_t) / sizeof(float);
	size = std::max(size, 2 * (long int) l);	// cache must be large enough for two columns
	lru_head.next = lru_head.prev = &lru_head;
}

Cache::~Cache()
{
	for(head_t *h = lru_head.next; h != &lru_head; h=h->next)
		free(h->data);
	free(head);
}

void Cache::lru_delete(head_t *h)
{
	// delete from current location
	h->prev->next = h->next;
	h->next->prev = h->prev;
}

void Cache::lru_insert(head_t *h)
{
	// insert to last position
	h->next = &lru_head;
	h->prev = lru_head.prev;
	h->prev->next = h;
	h->next->prev = h;
}

int Cache::get_data(const int index, float **data, int len)
{
	head_t *h = &head[index];
	if(h->len) lru_delete(h);
	int more = len - h->len;

	if(more > 0)
	{
		// free old space
		while(size < more)
		{
			head_t *old = lru_head.next;
			lru_delete(old);
			free(old->data);
			size += old->len;
			old->data = 0;
			old->len = 0;
		}

		// allocate new space
		h->data = (float *)realloc(h->data,sizeof(float)*len);
		size -= more;
		std::swap(h->len,len);
	}

	lru_insert(h);
	*data = h->data;
	return len;
}


class SVR_Q
{
private:
	std::vector<std::vector<svm_node>> x;

	// svm_parameter
	int kernel_type;
	int degree;
	double gamma;
	double coef0;

public:
	SVR_Q(const svm_problem& prob, const svm_parameter& param) {

        kernel_type = param.kernel_type;
		degree = param.degree;
		gamma = param.gamma;
		coef0 = param.coef0;

		x = prob.x;
		l = prob.l;

		cache = new Cache(l,(long int)(param.cache_size*(1<<20)));

		QD = new double[2*l];
		sign = new char[2*l];
		index = new int[2*l];

		for(int k=0;k<l;k++) {
			sign[k] = 1;
			sign[k+l] = -1;
			index[k] = k;
			index[k+l] = k;
			QD[k] = kernel_linear(k,k);
			QD[k+l] = QD[k];
		}
		buffer[0] = new float[2*l];
		buffer[1] = new float[2*l];
		next_buffer = 0;
	}

	void swap_index(int i, int j) const
	{
		std::swap(sign[i],sign[j]);
		std::swap(index[i],index[j]);
		std::swap(QD[i],QD[j]);
	}

	float *get_Q(int i, int len)
	{
		float *data;
		int j, real_i = index[i];
		if(cache->get_data(real_i,&data,l) < l)
		{
			for(j=0;j<l;j++)
				data[j] = (float)kernel_linear(real_i,j);
		}

		// reorder and copy
		float *buf = buffer[next_buffer];
		next_buffer = 1 - next_buffer;
		char si = sign[i];
		for(j=0;j<len;j++)
			buf[j] = (float) si * (float) sign[j] * data[index[j]];
		return buf;
	}

	double *get_QD() const
	{
		return QD;
	}

	static double k_function(const std::vector<svm_node> x, const std::vector<svm_node> y,
							  const svm_parameter& param)
	{
		switch(param.kernel_type)
		{
			case LINEAR:
				return dot(x,y);
			default:
				return 0;  // Unreachable
		}
	}

	double kernel_linear(int i, int j)
	{
		return dot(x[i],x[j]);
	}


	static double dot(const std::vector<svm_node> px, const std::vector<svm_node> py)
	{
		double sum = 0;
		int i = 0;
		int j = 0;
		while(px[i].index != -1 && py[j].index != -1)
		{
			if(px[i].index == py[j].index)
			{
				sum += px[i].value * py[j].value;
				i++;
				j++;
			}
			else
			{
				if(px[i].index > py[j].index)
					j++;
				else
					i++;
			}
		}
		return sum;
	}

	~SVR_Q()
	{
		delete cache;
		delete[] sign;
		delete[] index;
		delete[] buffer[0];
		delete[] buffer[1];
		delete[] QD;
	}
private:
	int l;
	Cache *cache;
	char *sign;
	int *index;
	mutable int next_buffer;
	float *buffer[2];
	double *QD;
};


class Solver {
public:
	Solver() {};
	virtual ~Solver() {};

	struct SolutionInfo {
		double obj;
		double rho;
		double upper_bound_p;
		double upper_bound_n;
		double r;	// for Solver_NU
	};

	void Solve(int l, SVR_Q& Q, const std::vector<double> &p_, const std::vector<char> &y_,
			   std::vector<double> &alpha_, double Cp, double Cn, double eps,
			   SolutionInfo &si, int shrinking);
protected:
	int active_size;
	std::vector<char> y;
	double *G;		// gradient of objective function
	enum { LOWER_BOUND, UPPER_BOUND, FREE };
	char *alpha_status;	// LOWER_BOUND, UPPER_BOUND, FREE
	std::vector<double> alpha;
	SVR_Q *Q;
	const double *QD;
	double eps;
	double Cp,Cn;
	std::vector<double> p;
	int *active_set;
	double *G_bar;		// gradient, if we treat free variables as 0
	int l;
	bool unshrink;	// XXX

	double get_C(int i)
	{
		return (y[i] > 0)? Cp : Cn;
	}
	void update_alpha_status(int i)
	{
		if(alpha[i] >= get_C(i))
			alpha_status[i] = UPPER_BOUND;
		else if(alpha[i] <= 0)
			alpha_status[i] = LOWER_BOUND;
		else alpha_status[i] = FREE;
	}
	bool is_upper_bound(int i) { return alpha_status[i] == UPPER_BOUND; }
	bool is_lower_bound(int i) { return alpha_status[i] == LOWER_BOUND; }
	bool is_free(int i) { return alpha_status[i] == FREE; }
	void swap_index(int i, int j);
	void reconstruct_gradient();
	virtual int select_working_set(int &i, int &j);
	virtual double calculate_rho();
	virtual void do_shrinking();
private:
	bool be_shrunk(int i, double Gmax1, double Gmax2);
};

void Solver::swap_index(int i, int j)
{
	Q->swap_index(i,j);
	std::swap(y[i],y[j]);
	std::swap(G[i],G[j]);
	std::swap(alpha_status[i],alpha_status[j]);
	std::swap(alpha[i],alpha[j]);
	std::swap(p[i],p[j]);
	std::swap(active_set[i],active_set[j]);
	std::swap(G_bar[i],G_bar[j]);
}

void Solver::reconstruct_gradient()
{
	// reconstruct inactive elements of G from G_bar and free variables

	if(active_size == l) return;

	int i,j;
	int nr_free = 0;

	for(j=active_size;j<l;j++)
		G[j] = G_bar[j] + p[j];

	for(j=0;j<active_size;j++)
		if(is_free(j))
			nr_free++;

	if(2*nr_free < active_size)
		info("\nWARNING: using -h 0 may be faster\n");

	if (nr_free*l > 2*active_size*(l-active_size))
	{
		for(i=active_size;i<l;i++)
		{
			const float *Q_i = Q->get_Q(i,active_size);
			for(j=0;j<active_size;j++)
				if(is_free(j))
					G[i] += alpha[j] * Q_i[j];
		}
	}
	else
	{
		for(i=0;i<active_size;i++)
			if(is_free(i))
			{
				const float *Q_i = Q->get_Q(i,l);
				double alpha_i = alpha[i];
				for(j=active_size;j<l;j++)
					G[j] += alpha_i * Q_i[j];
			}
	}
}

void Solver::Solve(int l, SVR_Q& Q, const std::vector<double> &p_, const std::vector<char> &y_,
				   std::vector<double> &alpha_, double Cp, double Cn, double eps,
				   SolutionInfo &si, int shrinking)
{
	this->l = l;
	this->Q = &Q;
	QD=Q.get_QD();


//	clone(p, p_,l);
	p = p_;
	y = y_;
	alpha = alpha_;
	this->Cp = Cp;
	this->Cn = Cn;
	this->eps = eps;
	unshrink = false;

	// initialize alpha_status
	{
		alpha_status = new char[l];
		for(int i=0;i<l;i++)
			update_alpha_status(i);
	}

	// initialize active set (for shrinking)
	{
		active_set = new int[l];
		for(int i=0;i<l;i++)
			active_set[i] = i;
		active_size = l;
	}

	// initialize gradient
	{
		G = new double[l];
		G_bar = new double[l];
		int i;
		for(i=0;i<l;i++)
		{
			G[i] = p[i];
			G_bar[i] = 0;
		}
		for(i=0;i<l;i++)
			if(!is_lower_bound(i))
			{
				const float *Q_i = Q.get_Q(i,l);
				double alpha_i = alpha[i];
				int j;
				for(j=0;j<l;j++)
					G[j] += alpha_i*Q_i[j];
				if(is_upper_bound(i))
					for(j=0;j<l;j++)
						G_bar[j] += get_C(i) * Q_i[j];
			}
	}

	// optimization step

	int iter = 0;
	int max_iter = std::max(10000000, l>INT_MAX/100 ? INT_MAX : 100*l);
	int counter = std::min(l,1000)+1;

	while(iter < max_iter)
	{
		// show progress and do shrinking

		if(--counter == 0)
		{
			counter = std::min(l,1000);
			if(shrinking) do_shrinking();
			info(".");
		}

		int i,j;
		if(select_working_set(i,j)!=0)
		{
			// reconstruct the whole gradient
			reconstruct_gradient();
			// reset active set size and check
			active_size = l;
			info("*");
			if(select_working_set(i,j)!=0)
				break;
			else
				counter = 1;	// do shrinking next iteration
		}

		++iter;

		// update alpha[i] and alpha[j], handle bounds carefully

		const float *Q_i = Q.get_Q(i,active_size);
		const float *Q_j = Q.get_Q(j,active_size);

		double C_i = get_C(i);
		double C_j = get_C(j);

		double old_alpha_i = alpha[i];
		double old_alpha_j = alpha[j];

		if(y[i]!=y[j])
		{
			double quad_coef = QD[i]+QD[j]+2*Q_i[j];
			if (quad_coef <= 0)
				quad_coef = TAU;
			double delta = (-G[i]-G[j])/quad_coef;
			double diff = alpha[i] - alpha[j];
			alpha[i] += delta;
			alpha[j] += delta;

			if(diff > 0)
			{
				if(alpha[j] < 0)
				{
					alpha[j] = 0;
					alpha[i] = diff;
				}
			}
			else
			{
				if(alpha[i] < 0)
				{
					alpha[i] = 0;
					alpha[j] = -diff;
				}
			}
			if(diff > C_i - C_j)
			{
				if(alpha[i] > C_i)
				{
					alpha[i] = C_i;
					alpha[j] = C_i - diff;
				}
			}
			else
			{
				if(alpha[j] > C_j)
				{
					alpha[j] = C_j;
					alpha[i] = C_j + diff;
				}
			}
		}
		else
		{
			double quad_coef = QD[i]+QD[j]-2*Q_i[j];
			if (quad_coef <= 0)
				quad_coef = TAU;
			double delta = (G[i]-G[j])/quad_coef;
			double sum = alpha[i] + alpha[j];
			alpha[i] -= delta;
			alpha[j] += delta;

			if(sum > C_i)
			{
				if(alpha[i] > C_i)
				{
					alpha[i] = C_i;
					alpha[j] = sum - C_i;
				}
			}
			else
			{
				if(alpha[j] < 0)
				{
					alpha[j] = 0;
					alpha[i] = sum;
				}
			}
			if(sum > C_j)
			{
				if(alpha[j] > C_j)
				{
					alpha[j] = C_j;
					alpha[i] = sum - C_j;
				}
			}
			else
			{
				if(alpha[i] < 0)
				{
					alpha[i] = 0;
					alpha[j] = sum;
				}
			}
		}

		// update G

		double delta_alpha_i = alpha[i] - old_alpha_i;
		double delta_alpha_j = alpha[j] - old_alpha_j;

		for(int k=0;k<active_size;k++)
		{
			G[k] += Q_i[k]*delta_alpha_i + Q_j[k]*delta_alpha_j;
		}

		// update alpha_status and G_bar

		{
			bool ui = is_upper_bound(i);
			bool uj = is_upper_bound(j);
			update_alpha_status(i);
			update_alpha_status(j);
			int k;
			if(ui != is_upper_bound(i))
			{
				Q_i = Q.get_Q(i,l);
				if(ui)
					for(k=0;k<l;k++)
						G_bar[k] -= C_i * Q_i[k];
				else
					for(k=0;k<l;k++)
						G_bar[k] += C_i * Q_i[k];
			}

			if(uj != is_upper_bound(j))
			{
				Q_j = Q.get_Q(j,l);
				if(uj)
					for(k=0;k<l;k++)
						G_bar[k] -= C_j * Q_j[k];
				else
					for(k=0;k<l;k++)
						G_bar[k] += C_j * Q_j[k];
			}
		}
	}

	if(iter >= max_iter)
	{
		if(active_size < l)
		{
			// reconstruct the whole gradient to calculate objective value
			reconstruct_gradient();
			active_size = l;
			info("*");
		}
		fprintf(stderr,"\nWARNING: reaching max number of iterations\n");
	}

	// calculate rho

	si.rho = calculate_rho();

	// calculate objective value
	{
		double v = 0;
		int i;
		for(i=0;i<l;i++)
			v += alpha[i] * (G[i] + p[i]);

		si.obj = v/2;
	}

	// put back the solution
	{
		for(int i=0;i<l;i++)
			alpha_[active_set[i]] = alpha[i];
	}

	// juggle everything back
	/*{
		for(int i=0;i<l;i++)
			while(active_set[i] != i)
				swap_index(i,active_set[i]);
				// or Q.swap_index(i,active_set[i]);
	}*/

	si.upper_bound_p = Cp;
	si.upper_bound_n = Cn;

	info("\noptimization finished, #iter = %d\n",iter);

//	delete[] p;
//	delete[] y;
//	delete[] alpha;
	delete[] alpha_status;
	delete[] active_set;
	delete[] G;
	delete[] G_bar;
}

// return 1 if already optimal, return 0 otherwise
int Solver::select_working_set(int &out_i, int &out_j)
{
	// return i,j such that
	// i: maximizes -y_i * grad(f)_i, i in I_up(\alpha)
	// j: minimizes the decrease of obj value
	//    (if quadratic coefficeint <= 0, replace it with tau)
	//    -y_j*grad(f)_j < -y_i*grad(f)_i, j in I_low(\alpha)

	double Gmax = -INF;
	double Gmax2 = -INF;
	int Gmax_idx = -1;
	int Gmin_idx = -1;
	double obj_diff_min = INF;

	for(int t=0;t<active_size;t++)
		if(y[t]==+1)
		{
			if(!is_upper_bound(t))
				if(-G[t] >= Gmax)
				{
					Gmax = -G[t];
					Gmax_idx = t;
				}
		}
		else
		{
			if(!is_lower_bound(t))
				if(G[t] >= Gmax)
				{
					Gmax = G[t];
					Gmax_idx = t;
				}
		}

	int i = Gmax_idx;
	const float *Q_i = NULL;
	if(i != -1) // NULL Q_i not accessed: Gmax=-INF if i=-1
		Q_i = Q->get_Q(i,active_size);

	for(int j=0;j<active_size;j++)
	{
		if(y[j]==+1)
		{
			if (!is_lower_bound(j))
			{
				double grad_diff=Gmax+G[j];
				if (G[j] >= Gmax2)
					Gmax2 = G[j];
				if (grad_diff > 0)
				{
					double obj_diff;
					double quad_coef = QD[i]+QD[j]-2.0*y[i]*Q_i[j];
					if (quad_coef > 0)
						obj_diff = -(grad_diff*grad_diff)/quad_coef;
					else
						obj_diff = -(grad_diff*grad_diff)/TAU;

					if (obj_diff <= obj_diff_min)
					{
						Gmin_idx=j;
						obj_diff_min = obj_diff;
					}
				}
			}
		}
		else
		{
			if (!is_upper_bound(j))
			{
				double grad_diff= Gmax-G[j];
				if (-G[j] >= Gmax2)
					Gmax2 = -G[j];
				if (grad_diff > 0)
				{
					double obj_diff;
					double quad_coef = QD[i]+QD[j]+2.0*y[i]*Q_i[j];
					if (quad_coef > 0)
						obj_diff = -(grad_diff*grad_diff)/quad_coef;
					else
						obj_diff = -(grad_diff*grad_diff)/TAU;

					if (obj_diff <= obj_diff_min)
					{
						Gmin_idx=j;
						obj_diff_min = obj_diff;
					}
				}
			}
		}
	}

	if(Gmax+Gmax2 < eps || Gmin_idx == -1)
		return 1;

	out_i = Gmax_idx;
	out_j = Gmin_idx;
	return 0;
}

bool Solver::be_shrunk(int i, double Gmax1, double Gmax2)
{
	if(is_upper_bound(i))
	{
		if(y[i]==+1)
			return(-G[i] > Gmax1);
		else
			return(-G[i] > Gmax2);
	}
	else if(is_lower_bound(i))
	{
		if(y[i]==+1)
			return(G[i] > Gmax2);
		else
			return(G[i] > Gmax1);
	}
	else
		return(false);
}

void Solver::do_shrinking()
{
	int i;
	double Gmax1 = -INF;		// max { -y_i * grad(f)_i | i in I_up(\alpha) }
	double Gmax2 = -INF;		// max { y_i * grad(f)_i | i in I_low(\alpha) }

	// find maximal violating pair first
	for(i=0;i<active_size;i++)
	{
		if(y[i]==+1)
		{
			if(!is_upper_bound(i))
			{
				if(-G[i] >= Gmax1)
					Gmax1 = -G[i];
			}
			if(!is_lower_bound(i))
			{
				if(G[i] >= Gmax2)
					Gmax2 = G[i];
			}
		}
		else
		{
			if(!is_upper_bound(i))
			{
				if(-G[i] >= Gmax2)
					Gmax2 = -G[i];
			}
			if(!is_lower_bound(i))
			{
				if(G[i] >= Gmax1)
					Gmax1 = G[i];
			}
		}
	}

	if(unshrink == false && Gmax1 + Gmax2 <= eps*10)
	{
		unshrink = true;
		reconstruct_gradient();
		active_size = l;
		info("*");
	}

	for(i=0;i<active_size;i++)
		if (be_shrunk(i, Gmax1, Gmax2))
		{
			active_size--;
			while (active_size > i)
			{
				if (!be_shrunk(active_size, Gmax1, Gmax2))
				{
					swap_index(i,active_size);
					break;
				}
				active_size--;
			}
		}
}

double Solver::calculate_rho()
{
	double r;
	int nr_free = 0;
	double ub = INF, lb = -INF, sum_free = 0;
	for(int i=0;i<active_size;i++)
	{
		double yG = y[i]*G[i];

		if(is_upper_bound(i))
		{
			if(y[i]==-1)
				ub = std::min(ub,yG);
			else
				lb = std::max(lb,yG);
		}
		else if(is_lower_bound(i))
		{
			if(y[i]==+1)
				ub = std::min(ub,yG);
			else
				lb = std::max(lb,yG);
		}
		else
		{
			++nr_free;
			sum_free += yG;
		}
	}

	if(nr_free>0)
		r = sum_free/nr_free;
	else
		r = (ub+lb)/2;

	return r;
}

//
// Solver for nu-svm classification and regression
//
// additional constraint: e^T \alpha = constant
//
class Solver_NU: public Solver
{
public:
	Solver_NU() {}
	void Solve(int l, SVR_Q& Q, const std::vector<double> &p, const std::vector<char> &y,
			   std::vector<double> &alpha, double Cp, double Cn, double eps,
			   SolutionInfo &si, int shrinking)
	{
		this->si = si;
		Solver::Solve(l,Q,p,y,alpha,Cp,Cn,eps,si,shrinking);
	}
private:
	SolutionInfo si;
	int select_working_set(int &i, int &j);
	double calculate_rho();
	bool be_shrunk(int i, double Gmax1, double Gmax2, double Gmax3, double Gmax4);
	void do_shrinking();
};

// return 1 if already optimal, return 0 otherwise
int Solver_NU::select_working_set(int &out_i, int &out_j)
{
	// return i,j such that y_i = y_j and
	// i: maximizes -y_i * grad(f)_i, i in I_up(\alpha)
	// j: minimizes the decrease of obj value
	//    (if quadratic coefficeint <= 0, replace it with tau)
	//    -y_j*grad(f)_j < -y_i*grad(f)_i, j in I_low(\alpha)

	double Gmaxp = -INF;
	double Gmaxp2 = -INF;
	int Gmaxp_idx = -1;

	double Gmaxn = -INF;
	double Gmaxn2 = -INF;
	int Gmaxn_idx = -1;

	int Gmin_idx = -1;
	double obj_diff_min = INF;

	for(int t=0;t<active_size;t++)
		if(y[t]==+1)
		{
			if(!is_upper_bound(t))
				if(-G[t] >= Gmaxp)
				{
					Gmaxp = -G[t];
					Gmaxp_idx = t;
				}
		}
		else
		{
			if(!is_lower_bound(t))
				if(G[t] >= Gmaxn)
				{
					Gmaxn = G[t];
					Gmaxn_idx = t;
				}
		}

	int ip = Gmaxp_idx;
	int in = Gmaxn_idx;
	const float *Q_ip = NULL;
	const float *Q_in = NULL;
	if(ip != -1) // NULL Q_ip not accessed: Gmaxp=-INF if ip=-1
		Q_ip = Q->get_Q(ip,active_size);
	if(in != -1)
		Q_in = Q->get_Q(in,active_size);

	for(int j=0;j<active_size;j++)
	{
		if(y[j]==+1)
		{
			if (!is_lower_bound(j))
			{
				double grad_diff=Gmaxp+G[j];
				if (G[j] >= Gmaxp2)
					Gmaxp2 = G[j];
				if (grad_diff > 0)
				{
					double obj_diff;
					double quad_coef = QD[ip]+QD[j]-2*Q_ip[j];
					if (quad_coef > 0)
						obj_diff = -(grad_diff*grad_diff)/quad_coef;
					else
						obj_diff = -(grad_diff*grad_diff)/TAU;

					if (obj_diff <= obj_diff_min)
					{
						Gmin_idx=j;
						obj_diff_min = obj_diff;
					}
				}
			}
		}
		else
		{
			if (!is_upper_bound(j))
			{
				double grad_diff=Gmaxn-G[j];
				if (-G[j] >= Gmaxn2)
					Gmaxn2 = -G[j];
				if (grad_diff > 0)
				{
					double obj_diff;
					double quad_coef = QD[in]+QD[j]-2*Q_in[j];
					if (quad_coef > 0)
						obj_diff = -(grad_diff*grad_diff)/quad_coef;
					else
						obj_diff = -(grad_diff*grad_diff)/TAU;

					if (obj_diff <= obj_diff_min)
					{
						Gmin_idx=j;
						obj_diff_min = obj_diff;
					}
				}
			}
		}
	}

	if(std::max(Gmaxp+Gmaxp2,Gmaxn+Gmaxn2) < eps || Gmin_idx == -1)
		return 1;

	if (y[Gmin_idx] == +1)
		out_i = Gmaxp_idx;
	else
		out_i = Gmaxn_idx;
	out_j = Gmin_idx;

	return 0;
}

bool Solver_NU::be_shrunk(int i, double Gmax1, double Gmax2, double Gmax3, double Gmax4)
{
	if(is_upper_bound(i))
	{
		if(y[i]==+1)
			return(-G[i] > Gmax1);
		else
			return(-G[i] > Gmax4);
	}
	else if(is_lower_bound(i))
	{
		if(y[i]==+1)
			return(G[i] > Gmax2);
		else
			return(G[i] > Gmax3);
	}
	else
		return(false);
}

void Solver_NU::do_shrinking()
{
	double Gmax1 = -INF;	// max { -y_i * grad(f)_i | y_i = +1, i in I_up(\alpha) }
	double Gmax2 = -INF;	// max { y_i * grad(f)_i | y_i = +1, i in I_low(\alpha) }
	double Gmax3 = -INF;	// max { -y_i * grad(f)_i | y_i = -1, i in I_up(\alpha) }
	double Gmax4 = -INF;	// max { y_i * grad(f)_i | y_i = -1, i in I_low(\alpha) }

	// find maximal violating pair first
	int i;
	for(i=0;i<active_size;i++)
	{
		if(!is_upper_bound(i))
		{
			if(y[i]==+1)
			{
				if(-G[i] > Gmax1) Gmax1 = -G[i];
			}
			else	if(-G[i] > Gmax4) Gmax4 = -G[i];
		}
		if(!is_lower_bound(i))
		{
			if(y[i]==+1)
			{
				if(G[i] > Gmax2) Gmax2 = G[i];
			}
			else	if(G[i] > Gmax3) Gmax3 = G[i];
		}
	}

	if(unshrink == false && std::max(Gmax1+Gmax2,Gmax3+Gmax4) <= eps*10)
	{
		unshrink = true;
		reconstruct_gradient();
		active_size = l;
	}

	for(i=0;i<active_size;i++)
		if (be_shrunk(i, Gmax1, Gmax2, Gmax3, Gmax4))
		{
			active_size--;
			while (active_size > i)
			{
				if (!be_shrunk(active_size, Gmax1, Gmax2, Gmax3, Gmax4))
				{
					swap_index(i,active_size);
					break;
				}
				active_size--;
			}
		}
}

double Solver_NU::calculate_rho()
{
	int nr_free1 = 0,nr_free2 = 0;
	double ub1 = INF, ub2 = INF;
	double lb1 = -INF, lb2 = -INF;
	double sum_free1 = 0, sum_free2 = 0;

	for(int i=0;i<active_size;i++)
	{
		if(y[i]==+1)
		{
			if(is_upper_bound(i))
				lb1 = std::max(lb1,G[i]);
			else if(is_lower_bound(i))
				ub1 = std::min(ub1,G[i]);
			else
			{
				++nr_free1;
				sum_free1 += G[i];
			}
		}
		else
		{
			if(is_upper_bound(i))
				lb2 = std::max(lb2,G[i]);
			else if(is_lower_bound(i))
				ub2 = std::min(ub2,G[i]);
			else
			{
				++nr_free2;
				sum_free2 += G[i];
			}
		}
	}

	double r1,r2;
	if(nr_free1 > 0)
		r1 = sum_free1/nr_free1;
	else
		r1 = (ub1+lb1)/2;

	if(nr_free2 > 0)
		r2 = sum_free2/nr_free2;
	else
		r2 = (ub2+lb2)/2;

	si.r = (r1+r2)/2;
	return (r1-r2)/2;
}

static void solve_nu_svr(
		const svm_problem &prob,
		const svm_parameter &param,
		std::vector<double> &alpha,
		Solver::SolutionInfo &si)
{
	int l = prob.l;
	double C = param.C;
	std::vector<double> alpha2(2*l);
	std::vector<double> linear_term(2*l);
	std::vector<char> y(2*l);

	double sum = C * param.nu * l / 2;
	for(int i=0;i<l;i++)
	{
		alpha2[i] = alpha2[i+l] = std::min(sum,C);
		sum -= alpha2[i];

		linear_term[i] = - prob.y[i];
		y[i] = 1;

		linear_term[i+l] = prob.y[i];
		y[i+l] = -1;
	}

	Solver_NU s;
	SVR_Q svr_q(prob, param);
	s.Solve(2*l, svr_q, linear_term, y, alpha2, C, C, param.eps, si, param.shrinking);

	for(int i=0;i<l;i++)
		alpha[i] = alpha2[i] - alpha2[i+l];
}

//
// decision_function
//
struct decision_function
{
	std::vector<double> alpha;
	double rho;
};

static decision_function svm_train_one(svm_problem prob, svm_parameter param, double Cp, double Cn)
{
//	double *alpha = Malloc(double,prob.l);
	std::vector<double> alpha(prob.l, 0.0);
	Solver::SolutionInfo si;

	solve_nu_svr(prob, param, alpha, si);

	info("obj = %f, rho = %f\n",si.obj,si.rho);

	// output SVs

	int nSV = 0;
	int nBSV = 0;
	for(int i=0;i<prob.l;i++)
	{
		if(fabs(alpha[i]) > 0)
		{
			++nSV;
			if(prob.y[i] > 0)
			{
				if(fabs(alpha[i]) >= si.upper_bound_p)
					++nBSV;
			}
			else
			{
				if(fabs(alpha[i]) >= si.upper_bound_n)
					++nBSV;
			}
		}
	}

	info("nSV = %d, nBSV = %d\n",nSV,nBSV);

	decision_function f;
	f.alpha = alpha;
	f.rho = si.rho;
	return f;
}


// Return parameter of a Laplace distribution
static double svm_svr_probability(
		const svm_problem prob, const svm_parameter param)
{
	int nr_fold = 5;
	std::vector<double> ymv(prob.l);
	double mae = 0;

	svm_parameter newparam(param);
	newparam.probability = 0;
	svm_cross_validation(prob, newparam, nr_fold, ymv);

	for(int i=0;i<prob.l;i++)
	{
		ymv[i]=prob.y[i]-ymv[i];
		mae += fabs(ymv[i]);
	}

	mae /= prob.l;
	double std=sqrt(2*mae*mae);
	int count=0;

	mae=0;
	for(int i=0;i<prob.l;i++)
		if (fabs(ymv[i]) > 5*std)
			count=count+1;
		else
			mae+=fabs(ymv[i]);

	mae /= (prob.l-count);
	info("Prob. model for test data: target value = predicted value + z,\nz: Laplace distribution e^(-|z|/sigma)/(2sigma),sigma= %g\n",mae);
	return mae;
}


//
// Interface functions
//
svm_model svm_train(const svm_problem &prob, const svm_parameter &param)
{
	svm_model model;
	model.param = param;
	model.free_sv = 0;
	model.nr_class = 2;
    model.sv_coef = std::vector<std::vector<double>>(1);

    if(param.probability && param.svm_type == NU_SVR)
    {
        model.probA = std::vector<double>(1, svm_svr_probability(prob, param));
    }

    decision_function f = svm_train_one(prob, param,0,0);
    model.rho = std::vector<double>(1, f.rho);

    int nSV = 0;
    for(int i=0;i<prob.l;i++)
        if(fabs(f.alpha[i]) > 0) nSV++;

    model.l = nSV;
	model.SV = std::vector<std::vector<svm_node>>(nSV);
    model.sv_coef[0] = std::vector<double>(nSV, 0.0);
    model.sv_indices = std::vector<int>(nSV, 0);

    int j = 0;
    for(int i=0;i<prob.l;i++)
        if(fabs(f.alpha[i]) > 0)
        {
            model.SV[j] = prob.x[i];
            model.sv_coef[0][j] = f.alpha[i];
            model.sv_indices[j] = i+1;
            j++;
        }

	return model;
}

// Stratified cross validation
void svm_cross_validation(svm_problem prob, svm_parameter param, int nr_fold, std::vector<double> target)
{
	int i;
	int l = prob.l;

	std::vector<int> perm(l);

	nr_fold = std::max(nr_fold, l);
	std::vector<int> fold_start(nr_fold+1);

	// stratified cv may not give leave-one-out rate
	// Each class to l folds -> some folds may have zero elements

	for(i=0;i<l;i++) perm[i]=i;
	for(i=0;i<l;i++) {
		int j = i+rand()%(l-i);
		std::swap(perm[i],perm[j]);
	}
	for(i=0;i<=nr_fold;i++)
		fold_start[i]=i*l/nr_fold;

	for(i=0;i<nr_fold;i++)
	{
		int begin = fold_start[i];
		int end = fold_start[i+1];
		svm_problem subprob;

		subprob.l = l-(end-begin);
		subprob.x = std::vector<std::vector<svm_node>>(subprob.l);
        subprob.y = std::vector<double>(subprob.l);

		int k=0;
		for(int j=0;j<begin;j++)
		{
			subprob.x[k] = prob.x[perm[j]];
			subprob.y[k] = prob.y[perm[j]];
			k++;
		}
		for(int j=end;j<l;j++)
		{
			subprob.x[k] = prob.x[perm[j]];
			subprob.y[k] = prob.y[perm[j]];
			k++;
		}
		svm_model submodel = svm_train(subprob, param);

		for(int j=begin;j<end;j++)
			target[perm[j]] = svm_predict(submodel,prob.x[perm[j]]);
	}
}


double svm_predict_values(const svm_model &model, const std::vector<svm_node> x, std::vector<double> &dec_values)
{
	std::vector<double> sv_coef = model.sv_coef[0];
	double sum = 0;

	for(int i=0;i<model.l;i++)
		sum += sv_coef[i] * SVR_Q::k_function(x, model.SV[i],model.param);

	sum -= model.rho[0];
	dec_values[0] = sum;

	return sum;

}

double svm_predict(const svm_model &model, const std::vector<svm_node> x)
{
    std::vector<double> dec_values;

    dec_values.assign(1, 0.0);

	double pred_result = svm_predict_values(model, x, dec_values);

	return pred_result;
}
