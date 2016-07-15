#include "model.h"

Weights::Embeddings::Embeddings(const NpzConverter& model, const std::string &key)
: E_(model[key])
{}

Weights::GRU::GRU(const NpzConverter& model, const std::vector<std::string> &keys)
: W_(model[keys.at(0)]),
  B_(model.asVector(keys.at(1))),
  U_(model[keys.at(2)]),
  Wx_(model[keys.at(3)]),
  Bx1_(model.asVector(keys.at(4))),
  Bx2_(model.asVector(Bx1_.cols())),
  Ux_(model[keys.at(5)])
{}

//////////////////////////////////////////////////////////////////////////////

Weights::DecInit::DecInit(const NpzConverter& model)
: Wi_(model["ff_state_W"]),
  Bi_(model.asVector("ff_state_b"))
{}

Weights::DecGRU2::DecGRU2(const NpzConverter& model)
: W_(model["decoder_Wc"]),
  B_(model.asVector("decoder_b_nl")),
  U_(model["decoder_U_nl"]),
  Wx_(model["decoder_Wcx"]),
  Bx2_(model.asVector("decoder_bx_nl")),
  Bx1_(model.asVector(Bx2_.cols())),
  Ux_(model["decoder_Ux_nl"])
{}

Weights::DecAttention::DecAttention(const NpzConverter& model)
: V_(model("decoder_U_att", true)),
W_(model["decoder_W_comb_att"]),
B_(model.asVector("decoder_b_att")),
U_(model["decoder_Wc_att"]),
C_(model["decoder_c_tt"]) // scalar?
{}

Weights::DecSoftmax::DecSoftmax(const NpzConverter& model)
: W1_(model["ff_logit_lstm_W"]),
  B1_(model.asVector("ff_logit_lstm_b")),
  W2_(model["ff_logit_prev_W"]),
  B2_(model.asVector("ff_logit_prev_b")),
  W3_(model["ff_logit_ctx_W"]),
  B3_(model.asVector("ff_logit_ctx_b")),
  W4_(model["ff_logit_W"]),
  B4_(model.asVector("ff_logit_b"))
{}

//////////////////////////////////////////////////////////////////////////////

Weights::Weights(const NpzConverter& model, size_t device)
: encEmbeddings_(model, "Wemb"),
    encForwardGRU_(model, {"encoder_W", "encoder_b", "encoder_U", "encoder_Wx", "encoder_bx", "encoder_Ux"}),
    encBackwardGRU_(model, {"encoder_r_W", "encoder_r_b", "encoder_r_U", "encoder_r_Wx", "encoder_r_bx", "encoder_r_Ux"}),
    decEmbeddings_(model, "Wemb_dec"),
    decInit_(model),
    decGru1_(model, {"decoder_W", "decoder_b", "decoder_U", "decoder_Wx", "decoder_bx", "decoder_Ux"}),
    decGru2_(model),
    decAttention_(model),
    decSoftmax_(model),
    device_(device)
{
	//cerr << *this << endl;
}
