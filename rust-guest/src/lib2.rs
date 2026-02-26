
// Copyright 2024 Adobe
// All Rights Reserved.
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in
// accordance with the terms of the Adobe license agreement accompanying
// it.

//! # Trustmark
//!
//! An implementation of TrustMark watermarking for the Content Authenticity Initiative (CAI) in
//! Rust, as described in:
//!
//! ---
//!
//! **TrustMark - Universal Watermarking for Arbitrary Resolution Images**
//!
//! <https://arxiv.org/abs/2311.18297>
//!
//! [Tu Bui]<sup>1</sup>, [Shruti Agarwal]<sup>2</sup>, [John Collomosse]<sup>1,2</sup>
//!
//! <sup>1</sup>DECaDE Centre for the Decentralized Digital Economy, University of Surrey, UK.\
//! <sup>2</sup>Adobe Research, San Jose CA.
//!
//! ---
//!
//! This is a re-implementation of the [trustmark] Python library.
//!
//! [Tu Bui]: https://www.surrey.ac.uk/people/tu-bui
//! [Shruti Agarwal]: https://research.adobe.com/person/shruti-agarwal/
//! [John Collomosse]: https://www.collomosse.com/
//! [trustmark]: https://pypi.org/project/trustmark/
//!
//! ## Example
//!
//! ```rust
//! use trustmark::{Trustmark, Version, Variant};
//!
//! # fn main() {
//! let tm = Trustmark::new("./models", Variant::Q, Version::Bch5).unwrap();
//! let input = image::open("../images/ghost.png").unwrap();
//! let output = tm.encode("0010101".to_owned(), input, 0.95);
//! # }
//! ```
use std::path::Path;


use image::{DynamicImage, GenericImageView as _};
use crate::cosmonic::onnx_runtime::types::{GraphOptimizationLevel, Session};

use crate::{bits, image_processing, model};
use self::{bits::Bits, image_processing::ModelImage};

/// A loaded Trustmark model.
pub struct Trustmark {
    encoder: Session,
    decoder: Session,
    version: Version,
    variant: Variant,
}

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("watermark is corrupt or missing")]
    CorruptWatermark,
    #[error("onnx error: {0}")]
    // Ort(#[from] crate::cosmonic::onnx_runtime::types::Error),
    Ort(crate::cosmonic::onnx_runtime::types::Error),
    #[error("image processing error: {0}")]
    ImageProcessing(#[from] image_processing::Error),
    #[error("bits processing error: {0}")]
    Bits(bits::Error),
    #[error("invalid model variant")]
    InvalidModelVariant,
}

impl From<bits::Error> for Error {
    fn from(value: bits::Error) -> Self {
        match value {
            bits::Error::CorruptWatermark => Error::CorruptWatermark,
            err => Error::Bits(err),
        }
    }
}

impl From<crate::cosmonic::onnx_runtime::types::Error> for Error {
    fn from(value: crate::cosmonic::onnx_runtime::types::Error) -> Self {
        Error::Ort(value.to_string())
    }
}

pub use bits::Version;
pub use model::Variant;

impl Trustmark {
    /// Load a Trustmark model.
    pub fn new<P: AsRef<Path>>(
        models: P,
        variant: Variant,
        version: Version,
    ) -> Result<Self, Error> {
        use crate::cosmonic::onnx_runtime::types;

        let encoder = {
            let session_options = types::SessionOptions {
                graph_optimization_level: Some(types::GraphOptimizationLevel::Disabled),
                // intra_op_num_threads: Some(8),
            };
            let model_data: &[u8] = include_bytes!("./encoder_B_f32.disable.ort");
            let session = types::create_session(model_data, Some(session_options)).unwrap();
            session
        };
        let decoder = {
            let session_options = types::SessionOptions {
                graph_optimization_level: Some(types::GraphOptimizationLevel::Disabled),
                // intra_op_num_threads: Some(8),
            };
            let model_data: &[u8] = include_bytes!("./decoder_Q_f32.disable.ort");
            let session = types::create_session(model_data, Some(session_options)).unwrap();
            session
        };


        // let encoder = Session::builder()?
        //     // .with_optimization_level(GraphOptimizationLevel::Level3)?
        //     .with_optimization_level(GraphOptimizationLevel::Layout)?
        //     .with_intra_threads(8)?
        //     .commit_from_file(models.as_ref().join(variant.encoder_filename()))?;
        // let decoder = Session::builder()?
        //     // .with_optimization_level(GraphOptimizationLevel::Level3)?
        //     .with_optimization_level(GraphOptimizationLevel::Layout)?
        //     .with_intra_threads(8)?
        //     .commit_from_file(models.as_ref().join(variant.decoder_filename()))?;
        Ok(Self {
            encoder,
            decoder,
            version,
            variant,
        })
    }

    /// Encode a watermark into an image.
    ///
    /// `watermark` is a bitstring encoding the watermark identifier to encode. `img` is the image
    /// which will be watermarked. `strength` is a number between 0 and 1 indicating how strong the
    /// resulting watermark should be. 0.95 is a normal strength.
    pub fn encode(
        &self,
        watermark: String,
        img: DynamicImage,
        strength: f32,
    ) -> Result<DynamicImage, Error> {
        let (original_width, original_height) = img.dimensions();
        let aspect_ratio = original_width as f32 / original_height as f32;

        // the image is always encoded with size 256x256
        let encode_size = 256;
        // let encode_size = 128;

        let input_img: crate::cosmonic::onnx_runtime::types::Tensor =
            ModelImage(encode_size, self.variant, img.clone()).try_into()?;
        let bits: crate::cosmonic::onnx_runtime::types::Tensor =
            Bits::apply_error_correction_and_schema(watermark, self.version)?.into();
        // let outputs = self.encoder.run(crate::cosmonic::onnx_runtime::types::inputs![
        //     "onnx::Concat_0" => input_img,
        //     "onnx::Gemm_1" => bits,
        // ]?)?;

        let inputs = self.encoder.get_inputs();
        for input in inputs {
            println!("input: {}", input.0);
        }

        // let outputs = self.encoder.run(vec![
        //     ("image".to_string(), input_img),
        //     ("input_1".to_string(), bits),  // Add the bits input!

        // ], None)?;

        let input_bytes = input_img.get_data(None);
        let input_sample: Vec<f32> = input_bytes[0..80]
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes([c[0], c[1], c[2], c[3]]))
            .collect();
        println!("INPUT image sample (first 20 floats): {:?}", input_sample);

        let bits_bytes = bits.get_data(None);
        let bits_sample: Vec<f32> = bits_bytes[0..80.min(bits_bytes.len())]
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes([c[0], c[1], c[2], c[3]]))
            .collect();
        println!("INPUT bits sample (first 20 floats): {:?}", bits_sample);

        let outputs = self.encoder.run(vec![
            ("onnx::Concat_0".to_string(), input_img),
            ("onnx::Gemm_1".to_string(), bits),
        ], None)?;

        let output_img_raw = outputs.iter().find(|(name, _)| name == "image").unwrap().1.get_data(None);
        println!("Encoder output bytes: {}", output_img_raw.len());
        // Sample a few pixel values
        let sample: Vec<f32> = output_img_raw[0..80]
            .chunks_exact(4)
            .map(|c| f32::from_le_bytes([c[0], c[1], c[2], c[3]]))
            .collect();
        println!("Encoder output sample (first 20 floats): {:?}", sample);
        println!("Sample min: {}, max: {}",
            sample.iter().cloned().fold(f32::INFINITY, f32::min),
            sample.iter().cloned().fold(f32::NEG_INFINITY, f32::max));

        // let outputs = self.encoder.run(vec![
        //     ("image".to_string(), input_img),
        //     ("input_1".to_string(), bits),      // Alternative 1
        // ], None)?;
        // let output_img = outputs.iter().find(|(name, _)| name == "image").unwrap().1.try_extract_tensor::<f32>()?.to_owned();
        println!("outputs: {:?}", outputs);
        let output_img = outputs.iter().find(|(name, _)| name == "image").unwrap().1.get_data(None);
        // let output_img = ndarray::ArrayView::from_vec(output_img);
        // let output_img: Vec<f32> = output_img_bytes
        //     .chunks_exact(4)
        //     .map(|chunk| {
        //         let bytes: [u8; 4] = [chunk[0], chunk[1], chunk[2], chunk[3]];
        //         f32::from_le_bytes(bytes)
        //     })
        //     .collect();
        // let output_img = safe_transmute::transmute_vec(output_img).unwrap();

        let output_img: Vec<f32> = output_img
            .chunks_exact(4)
            .map(|chunk| {
                let bytes: [u8; 4] = [chunk[0], chunk[1], chunk[2], chunk[3]];
                f32::from_le_bytes(bytes)
            })
            .collect();

        // let output_img: Vec<f32> = output_img
        //     .chunks_exact(2)  // f16 is 2 bytes, not 4
        //     .map(|chunk| {
        //         let bytes: [u8; 2] = [chunk[0], chunk[1]];
        //         half::f16::from_le_bytes(bytes).to_f32()
        //     })
        //     .collect();

        // ADD THIS DEBUG:
        println!("output_img bytes len: {}", outputs.iter().find(|(name, _)| name == "image").unwrap().1.get_data(None).len());
        println!("output_img f32 vec len: {}", output_img.len());
        println!("expected for [1,3,256,256]: {}", 1 * 3 * 256 * 256);


        // let ssshape = (1, 3);
        // let ssshape = [1, 3];
        // let ssshape = ndarray::IxDyn(&ssshape);
        // let output_img = ndarray::Array::from_shape_vec(ssshape.clone(), output_img).unwrap();
        let ssshape = [1, 3, encode_size as usize, encode_size as usize];
        let ssshape = ndarray::IxDyn(&ssshape);
        let output_img = ndarray::Array::from_shape_vec(ssshape.clone(), output_img).unwrap();

        // Need to calculate and apply the residual.
        let input_img: crate::cosmonic::onnx_runtime::types::Tensor =
            ModelImage(encode_size, self.variant, img.clone()).try_into()?;

        // let input_img_array = input_img.try_extract_tensor::<f32>()?;
        // let input_img_array = input_img.get_data(None);
        // let input_img_array: Vec<f32> = safe_transmute::transmute_vec(input_img_array).unwrap();
        let input_img_array = input_img.get_data(None);
        // let input_img_array: Vec<f32> = input_img_array
        //     .chunks_exact(2)
        //     .map(|chunk| {
        //         let bytes: [u8; 2] = [chunk[0], chunk[1]];
        //         half::f16::from_le_bytes(bytes).to_f32()
        //     })
        //     .collect();
        let input_img_array: Vec<f32> = input_img_array
            .chunks_exact(4)  // f32 is 4 bytes
            .map(|chunk| {
                let bytes: [u8; 4] = [chunk[0], chunk[1], chunk[2], chunk[3]];
                f32::from_le_bytes(bytes)
            })
            .collect();

        let residual = (self.variant.strength_multiplier() * strength)
            * (output_img - ndarray::Array::from_shape_vec(ssshape, input_img_array).unwrap());

        // Residual should be small perturbations.
        let mut residual = residual.clamp(-0.2, 0.2);
        if (self.variant == Variant::Q && !(0.5..=2.0).contains(&aspect_ratio))
            || self.variant == Variant::P
        {
            residual = image_processing::remove_boundary_artifact(
                residual,
                (original_width as usize, original_height as usize),
                self.variant,
            );
        }

        let ModelImage(_, _, residual) = (encode_size, self.variant, residual).try_into()?;

        Ok(image_processing::apply_residual(img, residual))
    }

    /// Decode a watermark from an image.
    pub fn decode(&self, img: DynamicImage) -> Result<String, Error> {
        // P variant has a smaller decode size
        let decode_size = if self.variant == Variant::P { 224 } else { 256 };

        let img: crate::cosmonic::onnx_runtime::types::Tensor =
            ModelImage(decode_size, self.variant, img).try_into()?;

        let inputs = self.decoder.get_inputs();
        for input in inputs {
            println!("input: {}", input.0);
        }

        let outputs = self.decoder.run(vec![
            ("image".to_string(), img),
        ], None)?;
        // let watermark = outputs["output"].try_extract_tensor::<f32>()?.to_owned();
        // let watermark = outputs.iter().find(|(name, _)| name == "output").unwrap().1.try_extract_tensor::<f32>()?.to_owned();
        let watermark = outputs.iter().find(|(name, _)| name == "output").unwrap().1.get_data(None);


        // let watermark: Vec<f32> = safe_transmute::transmute_vec(watermark).unwrap();
        let watermark: Vec<f32> = watermark
            .chunks_exact(4)
            .map(|chunk| {
                let bytes: [u8; 4] = [chunk[0], chunk[1], chunk[2], chunk[3]];
                f32::from_le_bytes(bytes)
            })
            .collect();

        println!("Decoded watermark len: {}", watermark.len());
        println!("Decoded watermark values (first 20): {:?}", &watermark[..20.min(watermark.len())]);
        println!("Min: {}, Max: {}",
            watermark.iter().cloned().fold(f32::INFINITY, f32::min),
            watermark.iter().cloned().fold(f32::NEG_INFINITY, f32::max));



        let ssshape = [1, 100];
        // let watermark: ndarray::ArrayD<u8> = ndarray::Array::from_shape_vec(ndarray::IxDyn(&ssshape), watermark).unwrap();
        // let watermark: ndarray::ArrayD<f32> = watermark.iter().map(|v| *v as f32);
        let watermark: ndarray::ArrayD<f32> = ndarray::Array::from_shape_vec(ndarray::IxDyn(&ssshape), watermark).unwrap();
        // let watermark: ndarray::ArrayD<f32> = ndarray::Array::from_shape_vec(ndarray::IxDyn(&ssshape), watermark).unwrap();


        let watermark: Bits = watermark.try_into()?;
        Ok(watermark.get_data())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loading_models() {
        Trustmark::new("./models", Variant::Q, Version::Bch5).unwrap();
    }

    fn roundtrip(path: impl AsRef<Path>) {
        let tm = Trustmark::new("./models", Variant::Q, Version::Bch5).unwrap();
        let input = image::open(path.as_ref()).unwrap();
        let watermark = "1011011110011000111111000000011111011111011100000110110110111".to_owned();
        let encoded = tm.encode(watermark.clone(), input, 0.95).unwrap();
        encoded.to_rgba8().save("./test.png").unwrap();
        let input = image::open("./test.png").unwrap();
        let decoded = tm.decode(input).unwrap();
        assert_eq!(watermark, decoded);
    }

    #[test]
    fn roundtrip_ghost() {
        roundtrip("../images/ghost.png");
    }

    #[test]
    fn roundtrip_ufo() {
        roundtrip("../images/ufo_240.jpg");
    }
}
