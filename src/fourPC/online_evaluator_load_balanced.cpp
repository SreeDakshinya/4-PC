#include "online_evaluator.h"

#include <array>

#include "../utils/helpers.h"

namespace graphsc
{
    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     int threads, int seed)
        : id_(id),
          rgen_(id, seed),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          wires_(circ.num_gates),
          q_sh_(circ.num_gates),
          q_val_(circ.num_gates)
    {
        tpool_ = std::make_shared<ThreadPool>(threads);
    }

    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     std::shared_ptr<ThreadPool> tpool, int seed)
        : id_(id),
          rgen_(id, seed),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          tpool_(std::move(tpool)),
          wires_(circ.num_gates),
          q_sh_(circ.num_gates),
          q_val_(circ.num_gates) {}

    void OnlineEvaluator::setInputs(const std::unordered_map<common::utils::wire_t, Ring> &inputs)
    {
        std::vector<Ring> masked_values;

        // Input gates have depth 0
        for (auto &g : circ_.gates_by_level[0])
        {
            if (g->type == common::utils::GateType::kInp)
            {
                auto *pre_input = static_cast<PreprocInput<Ring> *>(preproc_.gates[g->out].get());
                auto pid = pre_input->pid;

                if (id_ != 0)
                {
                    if (pid == id_)
                    {
                        Ring val;
                        rgen_.p12().random_data(&val, sizeof(Ring));
                        wires_[g->out] = inputs.at(g->out) - val;
                    }
                    else
                    {
                        Ring val;
                        rgen_.p12().random_data(&val, sizeof(Ring));
                        wires_[g->out] = val;
                    }
                }
            }
        }
    }

    void OnlineEvaluator::setRandomInputs()
    {
        // Input gates have depth 0.
        for (auto &g : circ_.gates_by_level[0])
        {
            if (g->type == common::utils::GateType::kInp)
            {
                Ring val;
                rgen_.p01().random_data(&val, sizeof(Ring));
                wires_[g->out] = val;
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepthPartySend(size_t depth,
                                                        std::vector<Ring> &mult_vals, std::vector<Ring> &mult_valsc)
    {

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kMul:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->out].get());
                    auto xa = pre_out->triple_a.valueAt() + wires_[g->in2];
                    auto yb = pre_out->triple_b.valueAt() + wires_[g->in2];
                    mult_vals.push_back(xa);
                    mult_vals.push_back(yb);
                    mult_valsc.push_back(pre_out->triple_c.valueAt());
                }

                break;
            }

            case ::common::utils::GateType::kAdd:
            case ::common::utils::GateType::kSub:
            case ::common::utils::GateType::kConstAdd:
            case ::common::utils::GateType::kConstMul:
            {
                break;
            }


            default:
                break;
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepthPartyRecv(size_t depth,
                                                        std::vector<Ring> mult_vals, std::vector<Ring> mult_valsc)
    {
        size_t idx_mult = 0;

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kAdd:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in1] + wires_[g->in2];
                break;
            }

            case common::utils::GateType::kSub:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in1] - wires_[g->in2];
                break;
            }

            case common::utils::GateType::kConstAdd:
            {
                auto *g = static_cast<common::utils::ConstOpGate<Ring> *>(gate.get());
                wires_[g->out] = wires_[g->in] + g->cval;
                break;
            }

            case common::utils::GateType::kConstMul:
            {
                auto *g = static_cast<common::utils::ConstOpGate<Ring> *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in] * g->cval;
                break;
            }

            case common::utils::GateType::kMul:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0)
                {
                    wires_[g->out] = mult_vals[2*idx_mult]*mult_vals[2*idx_mult + 1]*(id_-1) - mult_vals[2*idx_mult]*wires_[g->in2] - mult_vals[2*idx_mult+1]*wires_[g->in1] + mult_valsc[idx_mult];
                }
                idx_mult++;
                break;
            }

            default:
                break;
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepth(size_t depth)
    {
        size_t mult_num = 0;

        std::vector<Ring> mult_vals;
        std::vector<Ring> mult_valsc;

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kInp:
            case common::utils::GateType::kAdd:
            case common::utils::GateType::kSub:
            {
                break;
            }
            case common::utils::GateType::kMul:
            {
                mult_num++;
                break;
            }

            }
        }

        size_t total_comm = 2*mult_num;

        if (id_ != 0)
        {
            evaluateGatesAtDepthPartySend(depth, mult_vals, mult_valsc);

            
            if (id_ == 1)
                {
                    network_->send(2, mult_vals.data(), sizeof(Ring) * total_comm);
                    std::vector<Ring> recv_mult_vals(total_comm);
                    network_->recv(2, recv_mult_vals.data(), sizeof(Ring) * total_comm);
                    for (size_t i = 0; i < total_comm; i++){
                        mult_vals[i] = mult_vals[i] + recv_mult_vals[i];
                    }

                }
            if (id_ == 2)
                {
                    network_->send(1, mult_vals.data(), sizeof(Ring) * total_comm);
                    std::vector<Ring> recv_mult_vals(total_comm);
                    network_->recv(1, recv_mult_vals.data(), sizeof(Ring) * total_comm);
                    for (size_t i = 0; i < total_comm; i++){
                        mult_vals[i] = mult_vals[i] + recv_mult_vals[i];
                    }
                }



            evaluateGatesAtDepthPartyRecv(depth,
                                          mult_vals, mult_valsc);
        }
    }


    std::vector<Ring> OnlineEvaluator::getOutputs()
    {
        std::vector<Ring> outvals(circ_.outputs.size());
        if (circ_.outputs.empty())
        {
            return outvals;
        }

        if (id_ != 0)
        {
            std::vector<Ring> output_share_my(circ_.outputs.size());
            std::vector<Ring> output_share_other(circ_.outputs.size());
            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                auto wout = circ_.outputs[i];
                output_share_my[i] = wires_[wout];
                
            }

            if(id_ = 1){
                network_->send(2, output_share_my.data(), output_share_my.size() * sizeof(Ring));
                network_->recv(1, output_share_other.data(), output_share_other.size() * sizeof(Ring));
            }
            if(id_ = 2){
                network_->send(1, output_share_my.data(), output_share_my.size() * sizeof(Ring));
                network_->recv(2, output_share_other.data(), output_share_other.size() * sizeof(Ring));
            }

            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                Ring outmask = output_share_other[i];
                outvals[i] = output_share_my[i] + outmask ;
            }
            return outvals;
        }
        else
        {
            std::vector<Ring> output_masks(circ_.outputs.size());
            network_->recv(0, output_masks.data(), output_masks.size() * sizeof(Field));
            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                Ring outmask = output_masks[i];
                auto wout = circ_.outputs[i];
                outvals[i] = wires_[wout] - outmask;
            }
            return outvals;
        }
    }

    std::vector<Ring> OnlineEvaluator::evaluateCircuit(const std::unordered_map<common::utils::wire_t, Ring> &inputs)
    {
        setInputs(inputs);

        for (size_t i = 0; i < circ_.gates_by_level.size(); ++i)
        {
            evaluateGatesAtDepth(i);
        }

        return getOutputs();

    }

   
}; // namespace graphsc
