#include "animated_character.h"
#include "amc_util.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>


AnimatedCharacter::AnimatedCharacter(const std::string &asf_filename) :
    fps_(120.0), elapsed_since_last_frame_(0.0), current_frame_(0)
{
    LoadSkeleton(asf_filename);
}

AnimatedCharacter::AnimatedCharacter() :
    fps_(120.0), elapsed_since_last_frame_(0.0), current_frame_(0)
{
}

AnimatedCharacter::~AnimatedCharacter() {
}


void AnimatedCharacter::LoadSkeleton(const std::string &asf_filename) {
    skeleton_.LoadFromASF(asf_filename);
}


void AnimatedCharacter::Play(const MotionClip &motion_clip) {
    motion_queue_.clear();
    motion_queue_.push_back(motion_clip);
    current_frame_ = 0;
}


void AnimatedCharacter::Queue(const MotionClip &motion_clip) {
    if (motion_queue_.size() == 0) {
        Play(motion_clip);
    }
    else {
        motion_queue_.push_back(motion_clip);
    }
}


void AnimatedCharacter::ClearQueue() {
    motion_queue_.clear();
}


void AnimatedCharacter::OverlayClip(const MotionClip &clip, int num_transition_frames) {
    overlay_clip_ = clip;
    overlay_transition_frames_ = num_transition_frames;
    overlay_frame_ = 0;
}


void AnimatedCharacter::AdvanceAnimation(double dt) {
    if (motion_queue_.size() == 0) {
        pose_ = Pose();
    }
    else {
        elapsed_since_last_frame_ += dt;
        
        double frames_to_advance = fps_ * elapsed_since_last_frame_;
        double whole_frames;
        double frac = modf(frames_to_advance, &whole_frames);
        int nframes = (int)whole_frames;
        elapsed_since_last_frame_ = frac / fps_;
        
        for (int i=0; i<nframes; i++) {
            // advance the main motion track
            current_frame_++;
            // handle end case
            if (current_frame_ >= motion_queue_[0].size()) {
                // loop back to the first frame
                current_frame_ = 0;
                // if there are more motions in the queue then pop this one and goto the next
                if (motion_queue_.size() > 1) {
                    motion_queue_.erase(motion_queue_.begin());
                }
            }
            
            // advance the overlay clip if there is one
            if (overlay_clip_.size()) {
                overlay_frame_++;
                // handle end case
                if (overlay_frame_ >= overlay_clip_.size()) {
                    // done playing overlay, reset frame counter and clear the overlay clip
                    overlay_frame_ = 0;
                    overlay_clip_ = MotionClip();
                }
            }
            
            // update the pose based on new frames
            CalcCurrentPose();

            // add to the translation matrix for the case when relative root motion is used
            accum_translation_matrix_ = accum_translation_matrix_ * pose_.root_relative_translation();
        }
    }
}


void AnimatedCharacter::CalcCurrentPose() {
    if (!overlay_clip_.size()) {
        // no overaly track, motion is entirely from the base track (i.e., the motion queue)
        pose_ = motion_queue_[0][current_frame_];
    }
    else {
        // there is an active overlay track
        if (overlay_frame_ < overlay_transition_frames_) {
            // fade in the overlay
            float alpha = (float)overlay_frame_/(float)overlay_transition_frames_;
            pose_ = motion_queue_[0][current_frame_].Lerp(overlay_clip_[overlay_frame_], alpha);
        }
        else if (overlay_frame_ > overlay_clip_.size() - overlay_transition_frames_) {
            // fade out the overlay
            float alpha = (float)(overlay_clip_.size() - overlay_frame_)/(float)overlay_transition_frames_;
            pose_ = motion_queue_[0][current_frame_].Lerp(overlay_clip_[overlay_frame_], alpha);
        }
        else {
            // overlay is completely faded in, we don't see the base track at all
            pose_ = overlay_clip_[overlay_frame_];
        }
    }
}


Skeleton* AnimatedCharacter::skeleton_ptr() {
    return &skeleton_;
}


void AnimatedCharacter::set_fps(int fps) {
    fps_ = fps;
}


int AnimatedCharacter::fps() {
    return fps_;
}



void AnimatedCharacter::Draw(const Matrix4 &model_matrix, const Matrix4 &view_matrix, const Matrix4 &proj_matrix,
                             bool use_absolute_position)
{
    Matrix4 character_root_transform;
    if (use_absolute_position) {
        // set root position based on the absolute position in the mocap data
        character_root_transform = model_matrix * pose_.RootTransform();
    }
    else {
        // set root position based on the relative updates accumulated each frame
        character_root_transform = model_matrix * accum_translation_matrix_ * pose_.root_rotation();
    }
    
    for (int i=0; i<skeleton_.num_root_bones(); i++) {
        DrawBoneRecursive(skeleton_.root_bone(i), character_root_transform, view_matrix, proj_matrix);
    }
}


void AnimatedCharacter::DrawBoneRecursive(const std::string &bone_name, const Matrix4 &parent_transform,
                                          const Matrix4 &view_matrix, const Matrix4 &proj_matrix)
{
    // Step 1:  Draw this bone
    
    Matrix4 ctm = parent_transform;

    
    Vector3 bone = skeleton_.BoneDirectionAndLength(bone_name);

    Matrix4 rotation_space = skeleton_.BoneSpaceToRotAxesSpace(bone_name);
    Matrix4 bone_space = skeleton_.RotAxesSpaceToBoneSpace(bone_name);

    Matrix4 pose_rotation = pose_.JointRotation(bone_name);

    quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space, view_matrix, proj_matrix, Color(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0) + bone, 0.013);

    
    // TODO: Eventually, you'll want to draw something different depending on which part
    // of the body is being drawn.  An if statement like this is an easy way to do that.
    if (bone_name == "lhipjoint" || bone_name == "rhipjoint") {
    }
    if (bone_name == "lfemur" || bone_name == "rfemur") {
    }
    if (bone_name == "ltibia" || bone_name == "rtibia") {
    }
    if (bone_name == "lfoot" || bone_name == "rfoot") {
    }
    if (bone_name == "ltoes" || bone_name == "rtoes") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.05, 0.05, 0.05));
        Matrix4 sphere_translate = Matrix4::Translation(bone / 2);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));
    }
    if (bone_name == "lowerback") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.07, 0.23, 0.07));
        Matrix4 sphere_translate = Matrix4::Translation(-bone / 2.0);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));
    }
    if (bone_name == "upperback") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.05, 0.05, 0.05));
        Matrix4 sphere_translate = Matrix4::Translation(bone / 2);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));
    }
    if (bone_name == "thorax") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.07, 0.13, 0.07));
        Matrix4 sphere_translate = Matrix4::Translation(bone / 2);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));
    }
    if (bone_name == "lowerneck" || bone_name == "upperneck") {
        quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space, view_matrix, proj_matrix, Color(0.7, 0.0, 0.0), Point3(0.0, 0.0, 0.0), Point3(0.0, 0.0, 0.0) + bone, 0.017);
    }
    if (bone_name == "head") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.07, 0.13, 0.07));
        Matrix4 sphere_translate = Matrix4::Translation(bone / 2);
        Matrix4 sphere_rotate = Matrix4::RotationX(-M_PI_4);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_rotate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));

        Matrix4 antenna_matrix = Matrix4::Translation(Vector3(0.0, 0.11, 0.0));
        Matrix4 rotate_matrix = Matrix4::RotationZ(0.23);
        quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space * rotate_matrix * antenna_matrix, view_matrix, proj_matrix, Color(0, 0, 0), Point3(0, 0, 0), Point3(0, 0.25, 0.1), 0.007);
        quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space * rotate_matrix * antenna_matrix, view_matrix, proj_matrix, Color(0, 0, 0), Point3(0, 0.25, 0.1), Point3(0, 0.25, 0.18), 0.007);
        rotate_matrix = Matrix4::RotationZ(-0.23);
        quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space * rotate_matrix * antenna_matrix, view_matrix, proj_matrix, Color(0, 0, 0), Point3(0, 0, 0), Point3(0, 0.25, 0.1), 0.007);
        quick_shapes_.DrawLineSegment(ctm * bone_space * pose_rotation * rotation_space * rotate_matrix * antenna_matrix, view_matrix, proj_matrix, Color(0, 0, 0), Point3(0, 0.25, 0.1), Point3(0, 0.25, 0.18), 0.007);
    }
    if (bone_name == "lclavicle" || bone_name == "rclavicle") {
    }
    if (bone_name == "lhumerus" || bone_name == "rhumerus" || bone_name == "lradius" || bone_name == "rradius") {
    }
    if (bone_name == "lwrist" || bone_name == "rwrist") {
    }
    if (/*bone_name == "lhand" || bone_name == "rhand" || bone_name == "lthumb" || bone_name == "rthumb" ||*/ bone_name == "rfingers" || bone_name == "lfingers") {
        Matrix4 sphere_scale = Matrix4::Scale(Vector3(0.07, 0.07, 0.07));
        Matrix4 sphere_translate = Matrix4::Translation(bone / 2);
        quick_shapes_.DrawSphere(ctm * bone_space * pose_rotation * rotation_space * sphere_translate * sphere_scale, view_matrix, proj_matrix, Color(1.0, 0.0, 0.0));
    }
    
    
    // Step 2: Draw the bone's children
     
    Matrix4 child_root_transform = ctm * bone_space * pose_rotation * rotation_space * skeleton_.BoneSpaceToChildrenSpace(bone_name);
     
    for (int i=0; i<skeleton_.num_children(bone_name); i++) {
        DrawBoneRecursive(skeleton_.child_bone(bone_name, i), child_root_transform, view_matrix, proj_matrix);
    }

    // Second set of arms (these *are* ants)
    if (bone_name == "lowerback") {
        Matrix4 scale = Matrix4::Scale(Vector3(0.7, 0.7, 0.7));
        DrawBoneRecursive("lclavicle", child_root_transform * scale, view_matrix, proj_matrix);
        DrawBoneRecursive("rclavicle", child_root_transform * scale, view_matrix, proj_matrix);
    }
}



